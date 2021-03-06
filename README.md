FreeBits
=======

Compiling and Running
To compile type "make". Note this will remove all previously existing output 
files like *.out   *-_csu_cs556_*.txt. If you don't want these removed, compile 
with "make no-cleanup".
"make clean" will remove all executable, *.o, and other output files.


Reused Code

		Beej's Guide to Network Programming was an asset in completing this assignment.
	Since this way my first time programming c-sockets, I found this assignment to 
	have a very slow start. Much (if not all) of the socket programming came out of
	this book as a result. Other than the socket programming, the only other code 
	use was the provided timer libraries. These were used as the main control loop
	for the client. The timer event queue issued interest requests to the track at
	the specified times and provided means for termination (expained in the 
	termination section later). The changes I made to it were just to implement my
	own timer event that would talk to the tracker to express an interest and to 
	insert my own packet processing into the main loop.

	Beej's Guide to Network Programming -- http://beej.us/guide/bgnet/

Message formats

	I modified the designated client tracker message formats in two ways:

	1.) In the Client -> Tracker message, I added a field denoting the clients listening
		port number so the tracker could provide that information in its responses to 
		interest requests.

	2.) I re-ordered both client -> tracker and tracker -> client messages to a format
		where all of the filenames, type, ids, ports, etc.. would be sequential rather
		than interleaved (ie all of the file names, then all of the neighbor ids, etc)
		for ease of both sending and recieving such packets.

	My client-to-client protocol consisted of 4 distinct messages: info request, 
	info response, data request, data response. 

	My design attempted to keep the message sizes small and efficient to process. To
	do this I devised the following scheme to communicate file segment information: 
	using a variable sized bit-wise word, write 1 bit per file segment (1 meaning 
	has and 0 meaning does not have). This will communicate not only which segments 
	a node possesses, but the file size as well. Each node will maintain a bit-wise
	segment word for each file they have (or will download). To quickly identify which
	segments a node has that should be downloaded a simple bitwise operation can be
	performed:

	[NOT(My segment word)] AND [Their segment word]
	For example:
	   Mine = 11101101
	 Theirs = 00011110

	NOT(Mine) = 00010010
	   Thiers = 00011110
		    AND ________
	   Result:  00010010

	So for the file, "I" need the segments coresponding to the 1's in the result from
	the other node.


	I will depict each format and comment
	on them below:

	********************************
	* INFO REQUEST/RESPONSE PACKET *
	********************************
	 0                   1                   2                   3 
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+---------------+---------------+----------------+--------------+
	|       MESSAGE TYPE            |         SRC NODE ID           |
	+---------------+---------------+----------------+--------------+
	|        SRC PORT 				|                               
	+---------------+---------------+----------------+--------------+
	|                                                               |
	|                                                               |
	|                                                               |
	|       FILENAME (32 bytes)                                     |
	|                                                               |
	|                                                               |
	|                                                               |
	+---------------+---------------+----------------+--------------+
	|          TOTAL SEGMENTS       | 								
	+---------------+---------------+----------------+--------------+
	|     DOWNLOADED SEGMENTS  (total_segment bits)                 |
	|    ...      ...         ...      ...         ...      ...     |
	+---------------+---------------+----------------+--------------+
	Field descriptions:
		Message type: int message identifier

		Src node id: node_id of the sender for booking purposes to the receiver

		Src port: same purpose as src node.

		Filename: the name of, you guessed it, the file of interest

		Total segments: To concisely send segment information over the wire, I devised
						the following scheme: using a variable sized word, write 1 bit
						per file segment (1 meaning has and 0 meaning does not have).
						This will tell the receiver not only which segments it 
						possesses, but the file size as well. This field just states 
						the size of the following word.

		Downloaded Segments: The word described above containing 1 bit per file segment,
							 1 meaning I have and 0 I don't have. 

	Comments:
		This packet is used for both info requests and responses with the message type 
		distiguishing between them. Each client will keep a bit-wise segment representation
		word of each file it has (or will download). Upon recieving this packet the 
		receiving client can keep track of what files/segments it's peers have and if it is a 
		info request packet, respond accordingly with its own info response message.


	***************************
	* DOWNLOAD REQUEST PACKET *
	***************************
	 0                   1                   2                   3 
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+---------------+---------------+----------------+--------------+
	|       MESSAGE TYPE            |         SRC NODE ID           |
	+---------------+---------------+----------------+--------------+
	|        SRC PORT 				|                               
	+---------------+---------------+----------------+--------------+
	|                                                               |
	|                                                               |
	|                                                               |
	|       FILENAME (32 bytes)                                     |
	|                                                               |
	|                                                               |
	|                                                               |
	+---------------+---------------+----------------+--------------+
	|      NUMBER OF SEGMENTS       | 								
	+---------------+---------------+----------------+--------------+
	|     SEGMENTS NUMBERS                                          |
	|    ...      ...         ...      ...         ...      ...     |
	+---------------+---------------+----------------+--------------+
	Field descriptions:
		Message type: int message identifier

		Src node id: node_id of the sender for booking purposes to the receiver

		Src port: same purpose as src node.

		Filename: the name of, you guessed it, the file of interest

		Number of segments: The number of segement numbers to follow in the next field

		Segment Numbers: The segment identifiers desired for the file.

	Comments:
		A pretty intuitive scheme, upon recieving such a packet, the specified segments 
		can be seeked to in the downloaded file and sent back to the source in a download
		response message.	



	****************************
	* DOWNLOAD RESPONSE PACKET *
	****************************
	 0                   1                   2                   3 
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+---------------+---------------+----------------+--------------+
	|       MESSAGE TYPE            |         SRC NODE ID           |                              
	+---------------+---------------+----------------+--------------+
	|                                                               |
	|                                                               |
	|                                                               |
	|       FILENAME (32 bytes)                                     |
	|                                                               |
	|                                                               |
	|                                                               |
	+---------------+---------------+----------------+--------------+
	|      NUMBER OF SEGMENTS       | 								
	+---------------+---------------+----------------+--------------+
	|     SEGMENT DATA                                              |
	|    ...      ...         ...      ...         ...      ...     |
	+---------------+---------------+----------------+--------------+
	Field descriptions:
		Message type: int message identifier

		Src node id: node_id of the sender for booking purposes to the receiver

		Filename: the name of, you guessed it, the file of interest

		Number of segments: The number of segement numbers to follow in the next field.

		Segment data: The series of 32 byte data segments in the order of the numbers
					specified in the download request packet.

	Comments:
		A pretty intuitive scheme, upon recieving such a packet, the specified segments 
		can be seeked to in the downloaded file and sent back to the source in a download
		response message.

	
Idiosyncrasies:
		There are a few idiosyncrasies this project has. Knowing the termination
	conditions, one could devise a manager.conf file that would exloit this
	and cause a node to terminate that would be needed at later time, thus 
	being unavailable. Also, attempting to send a and retrieve and empty file
	can cause trouble as it will appear as if no one has any segments (as none
	exist). I found this to be a pretty small flaw in that there is no point in
	sharing an empty file. 

	In terms of input, I believe the manager.conf file parser to be fairly resilient.
	As long as, the syntax described in the assignment are followed, variable amounts of
	white space, commenting, etc.. are handled and ignored or an error message occurs if
	it cannot continue without the info.

	The packet loss extra credit not attempted.

Protocol Corner Cases

  -- One to one transfer --
		One to one transfers provide the least amount of protential problems. As mentioned earlier,
	empty files with my client to client protocol will show that no nodes have any segments of
	the empty file and thus think it cannot progress. Doing a max size file transfer could potentially
	cause problems with packet sizes. Then there are timeout issues, such as a client requests the
	file before it available; and varients of this case. All of these scenarios apply to all of 
	the following cases.


  -- Many to Many transfer --
		With a many to many transfer things can get a little more tricky. There is more data to maintain
	and thus more things can go wrong. Requesting a non-existant file or, conversily, requsting
	duplicate files can cause some issues. Here with many-to-many willingness to share files now comes 
	into play. If a file shows interest and does not want to share they should not be sent info requests;
	and varients of the sort.

  -- Transfers with multiple files --
		Once multiple files are introduced, then we have to make sure that we have separation of conserns 
	for each file. One such case would be to have one node recieve many files from many different clients.

  -- Packet loss --
		Acconting for information that may have been lost will introduce a whole new set of problems.
	Cases where the info request/response packet is lost result in some challenges where we do not know
	whether they are lost or slow.

Termination

		My termination scheme has two parts. First, for a client node, I leveraged the timer event queue
	to ensure that clients with tasks still to perform do not die off. After a node has completed its
	specifed tasks it's event queue will be empty and its only job is to repsond to other nodes if need.
	If a client does not hear from any other node after ten seconds and its task queue is empty it will 
	terminate.

	The tracker will simply terminate if it has not heard from any of the clients in ten seconds. It will
	then terminate.

Surprises

		Well, certianly the time I spent on this project was surprising to say the least. As I mentioned 
	earlier this was my first time programming c-sockets, so this was a pretty brutal introduction to 
	them.. I come from a background in distributed systems so I wanted to apply many of those concepts,
	but was unable to due to constraints in the assignment description (ie. the packet formats). 

