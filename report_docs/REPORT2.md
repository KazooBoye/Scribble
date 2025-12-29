# BÁO CÁO CHI TIẾT DỰ ÁN
## ỨNG DỤNG TRÒ CHƠI VẼ VÀ ĐOÁN TỪ NHIỀU NGƯỜI CHƠI

**Môn học:** Lập trình mạng (Network Programming)  
**Người thực hiện:** Cao Duc Anh  
**Ngày báo cáo:** 28/12/2025

---

## I. GIỚI THIỆU ĐỀ TÀI

### 1.1. Bối cảnh và động lực

Trong bối cảnh công nghệ thông tin phát triển mạnh mẽ, các ứng dụng mạng đa người dùng thời gian thực đã trở thành một phần không thể thiếu trong cuộc sống hàng ngày. Từ các ứng dụng nhắn tin, hội nghị trực tuyến đến các trò chơi trực tuyến, tất cả đều yêu cầu khả năng xử lý kết nối đồng thời, đồng bộ hóa trạng thái giữa nhiều máy khách và đảm bảo độ tin cậy trong truyền thông. Đặc biệt trong lĩnh vực game multiplayer, việc thiết kế một hệ thống mạng hiệu quả là yếu tố quyết định trải nghiệm người dùng.

Dự án này được phát triển với mục đích nghiên cứu và thực hành các khái niệm cốt lõi trong lập trình mạng thông qua việc xây dựng một ứng dụng trò chơi vẽ và đoán từ nhiều người chơi (multiplayer drawing and guessing game), tương tự như các ứng dụng phổ biến như skribbl.io. Khác với các giải pháp web-based sử dụng WebSocket hoặc các abstraction layer cao cấp, dự án này tập trung vào việc sử dụng Berkeley Sockets API thuần túy với TCP socket, cho phép kiểm soát toàn bộ quá trình truyền thông ở tầng transport layer.

### 1.2. Mục tiêu và phạm vi

Mục tiêu chính của dự án là xây dựng một hệ thống client-server hoàn chỉnh có khả năng xử lý nhiều người chơi đồng thời trong một trò chơi thời gian thực. Hệ thống cần đảm bảo các yêu cầu kỹ thuật sau:

Thứ nhất, về mặt kiến trúc mạng, hệ thống phải sử dụng Berkeley Sockets API với TCP protocol để đảm bảo độ tin cậy trong việc truyền tải dữ liệu game logic, chat messages và các thông tin trạng thái quan trọng. Việc lựa chọn TCP thay vì UDP cho phép hệ thống tận dụng các cơ chế built-in như flow control, congestion control và automatic retransmission, đặc biệt quan trọng trong việc đồng bộ hóa trạng thái game giữa server và nhiều client.

Thứ hai, về khả năng mở rộng, server phải có khả năng xử lý đồng thời tối thiểu 100 kết nối TCP và quản lý tới 100 phòng chơi song song, mỗi phòng hỗ trợ từ 2 đến 5 người chơi. Điều này đòi hỏi việc thiết kế cẩn thận cơ chế I/O multiplexing và quản lý tài nguyên hiệu quả.

Thứ ba, về đồng bộ hóa dữ liệu, hệ thống cần đảm bảo rằng mọi thao tác vẽ từ người chơi được broadcast real-time đến tất cả thành viên trong phòng với độ trễ tối thiểu. Mỗi nét vẽ được mã hóa thành các thông điệp JSON chứa tọa độ, màu sắc và độ dày nét, sau đó được truyền qua TCP socket và render lại trên canvas của các client khác.

Thứ tư, về tính năng game logic, server phải quản lý toàn bộ luật chơi bao gồm hệ thống turn-based drawing (mỗi người vẽ một lượt), scoring system dựa trên thời gian đoán đúng, round progression và game end conditions. Server cũng chịu trách nhiệm xác thực các lời đoán từ người chơi bằng cách so sánh case-insensitive với từ cần vẽ.

Cuối cùng, về persistence và monitoring, hệ thống cần lưu trữ thống kê người chơi bao gồm số trận đã chơi, số trận thắng, tổng điểm tích lũy, số lần đoán đúng và thời gian đoán nhanh nhất vào file CSV với cơ chế thread-safe. Ngoài ra, hệ thống cũng triển khai connection monitoring với PING/PONG keepalive để phát hiện các kết nối bị ngắt và cập nhật trạng thái online/offline của người chơi.

### 1.3. Kiến trúc tổng quan

Dự án được thiết kế theo mô hình client-server architecture với kiến trúc phân lớp ba tầng ở application level: presentation layer (Pygame client), business logic layer (C server) và networking abstraction layer (C shared library). Cần lưu ý rằng đây là phân lớp trong kiến trúc ứng dụng (application-level architecture), khác với mô hình OSI 7 tầng. Trong mô hình OSI, toàn bộ ba thành phần này đều hoạt động tại tầng Application (Layer 7), trong khi giao tiếp mạng sử dụng TCP protocol tại tầng Transport (Layer 4) được cung cấp bởi hệ điều hành thông qua Berkeley Sockets API.

Server được viết hoàn toàn bằng C11 với POSIX threads để tận dụng tối đa hiệu năng của hệ điều hành, trong khi client sử dụng Python với Pygame framework để tạo giao diện đồ họa 2D. Sự kết nối giữa Python và C networking library được thực hiện thông qua ctypes Foreign Function Interface, cho phép Python code gọi trực tiếp các hàm C đã compile thành shared library.

Một điểm đặc biệt trong kiến trúc là việc tách biệt networking logic thành một shared library độc lập (libscribble_client.dylib trên macOS hoặc .so trên Linux). Library này đóng gói toàn bộ các thao tác socket level như connection management, message framing với 4-byte length prefix và error handling. Điều này không chỉ giúp tái sử dụng code giữa các client platform khác nhau mà còn cách ly các vấn đề về network programming khỏi game logic, giúp dễ dàng debug và maintain.

---

## II. PHÂN TÍCH THIẾT KẾ HỆ THỐNG

### 2.1. Kiến trúc phân lớp

**Vị trí trong mô hình OSI**

Trước khi đi vào chi tiết kiến trúc ứng dụng, cần làm rõ vị trí của hệ thống trong mô hình OSI 7 tầng. Toàn bộ game application (bao gồm client UI, business logic, và data persistence) hoạt động tại tầng Application Layer (Layer 7). Các tầng dưới được hệ điều hành và hardware xử lý: tầng Presentation và Session (Layer 6, 5) được skip trong hầu hết ứng dụng hiện đại, tầng Transport (Layer 4) sử dụng TCP protocol được kernel cung cấp thông qua Berkeley Sockets API, tầng Network (Layer 3) sử dụng IP protocol cho routing, và tầng Data Link/Physical (Layer 2, 1) xử lý Ethernet/WiFi frames và physical signaling.

Khi báo cáo nhắc đến "layers" hay "tầng" trong kiến trúc ứng dụng (presentation layer, business logic layer, data access layer), đây là phân lớp logic bên trong tầng Application của OSI, không phải các tầng riêng biệt trong OSI stack. Việc phân biệt này quan trọng để tránh nhầm lẫn giữa application architecture patterns và network protocol layering.

**Kiến trúc ứng dụng ba thành phần**

Hệ thống được tổ chức theo kiến trúc phân lớp với ba thành phần chính, trong đó server backend được cấu trúc thành nhiều tầng con để tách biệt rõ ràng các trách nhiệm. Mỗi thành phần giao tiếp với nhau thông qua các interface được định nghĩa chặt chẽ.

**Client Side - Presentation Layer (Pygame Application)**

Thành phần này chịu trách nhiệm toàn bộ việc tương tác với người dùng thông qua giao diện đồ họa. Client được xây dựng trên Pygame framework, một thư viện Python chuyên dụng cho phát triển game 2D, cho phép render graphics với hardware acceleration. Cửa sổ ứng dụng có kích thước cố định 1200x700 pixels, được chia thành các vùng chức năng rõ ràng: sidebar bên trái hiển thị danh sách người chơi với điểm số và trạng thái online, vùng canvas trung tâm kích thước 700x500 pixels dành cho việc vẽ, và khu vực chat ở phía dưới.

Client được thiết kế theo state machine pattern với bốn trạng thái chính: LANDING (màn hình khởi động cho phép người dùng nhập tên và chọn chế độ chơi), WAITING (phòng chờ hiển thị room code và countdown 15 giây), PLAYING (màn hình chơi chính với canvas, drawing tools và chat) và ENDED (màn hình kết thúc hiển thị bảng xếp hạng). Mỗi trạng thái có riêng event handlers và render logic, giúp code dễ maintain và extend.

Game loop chạy ở 60 FPS được điều khiển bởi pygame.time.Clock, đảm bảo frame rate ổn định trên các hệ thống khác nhau. Trong mỗi iteration của game loop, client thực hiện ba nhiệm vụ chính: xử lý pygame events (mouse clicks, keyboard input, window events), nhận và xử lý network messages một cách non-blocking thông qua C library, và cuối cùng là render toàn bộ game state lên screen. Drawing trên canvas được thực hiện bằng cách lưu trữ các stroke objects trong memory và re-render tất cả mỗi frame, đảm bảo consistency khi có stroke mới được broadcast từ server.

**Networking Abstraction Layer (C Shared Library)**

Thành phần này đóng vai trò cầu nối giữa Python client và C server, được implement hoàn toàn bằng C và compile thành shared library. Thư viện này cung cấp một interface đơn giản cho Python thông qua ctypes với các hàm cơ bản: network_connect() để thiết lập TCP connection đến server, network_send_tcp() để gửi message với length prefix framing, network_recv_tcp() để nhận message một cách non-blocking với timeout, và network_disconnect() để đóng connection một cách graceful.

Một trong những thiết kế quan trọng nhất của thư viện này là cơ chế message framing. Vì TCP là stream-oriented protocol không có khái niệm message boundaries, mỗi message được prefix bởi 4 bytes biểu diễn chiều dài của JSON payload theo định dạng big-endian (network byte order). Khi gửi, hàm network_send_tcp() sẽ serialize message length thành 4 bytes với htonl() function, sau đó concatenate với JSON string và gọi send() system call. Khi nhận, library duy trì một receive buffer và parse từng message hoàn chỉnh bằng cách đọc 4 bytes đầu để biết chiều dài, kiểm tra xem buffer có đủ data không, sau đó extract message và chuyển phần còn lại về đầu buffer.

Thư viện cũng triển khai một background receive thread chạy liên tục khi connection active. Thread này blocking trên recv() system call với timeout nhỏ (khoảng 100ms) để có thể kiểm tra stop flag định kỳ. Khi nhận được data, thread sẽ append vào buffer và parse tất cả complete messages, sau đó gọi callback function được register từ Python để deliver message. Thiết kế này cho phép Python code nhận message asynchronously mà không cần block main thread.

**Server Side - Complete Backend (C Application)**

Đây là thành phần core của hệ thống, được cấu trúc thành nhiều tầng con với trách nhiệm rõ ràng:

*Network Layer (TCP Server):* Module tcp_server.c chịu trách nhiệm quản lý toàn bộ kết nối mạng. Server khởi động bằng cách tạo một TCP listening socket bind vào port 9090 và set vào non-blocking mode. Main server thread chạy một infinite loop với select() system call để monitor tất cả active file descriptors (listening socket và các client sockets). Khi select() return, server kiểm tra xem event nào đã xảy ra: nếu listening socket ready thì accept new connection, nếu client socket ready thì read data và forward đến tầng trên để xử lý. Cách tiếp cận này cho phép server handle hàng trăm concurrent connections với chỉ một thread, tận dụng hiệu quả kernel-space multiplexing.

*Application Protocol Layer (Message Handlers):* Module tcp_handler.c đảm nhận việc parse và route messages. Mỗi client được represent bởi một Player struct chứa file descriptor, username, player_id, score, state và một receive buffer riêng. Receive buffer này quan trọng vì TCP có thể deliver data từng phần (partial receives), server cần accumulate data cho đến khi có đủ một complete message theo length-prefix protocol. Khi process message, server parse JSON payload để extract message type và data, sau đó route đến appropriate handler function trong tầng business logic. Module tcp_parser.c hỗ trợ việc serialize và deserialize messages theo protocol đã định nghĩa.

*Business Logic Layer (Game Engine):* Module game_logic.c implement toàn bộ luật chơi và game flow control. Game state được organize quanh khái niệm Room, mỗi room là một game session độc lập với tối đa 5 players. Room struct chứa array of player pointers, room_id, room_code (cho private rooms), current game state (WAITING, PLAYING, ENDED), current drawer index, word to draw, round number, timer và array of strokes cho canvas state. Module matchmaking.c xử lý việc tạo phòng và ghép người chơi, duy trì một global array of 100 rooms và sử dụng first-fit algorithm để assign players vào rooms khi matchmaking.

Game flow được điều khiển bởi một timer thread riêng chạy mỗi giây (1Hz tick rate). Thread này iterate qua tất cả active rooms và gọi callback functions để update countdown timer (khi đang chờ đủ người), update round timer (khi đang chơi), và check các end conditions. Khi round timer hết hoặc tất cả người chơi đã đoán đúng, server sẽ call end_round() function để calculate scores, broadcast kết quả và bắt đầu round tiếp theo. Sau khi tất cả rounds kết thúc, server call end_game() để broadcast final rankings và reset room state.

*Data Access Layer (Persistence):* Module stats.c quản lý việc lưu trữ và truy xuất thống kê người chơi vào file CSV (player_stats.txt). Module này implement thread-safe operations với pthread_mutex để đảm bảo data integrity khi nhiều game sessions kết thúc đồng thời. Statistics bao gồm số trận đã chơi, số trận thắng, tổng điểm, số lần đoán đúng, số round đã vẽ và thời gian đoán nhanh nhất. Module logger.c cung cấp chức năng logging các events quan trọng của game dưới định dạng JSON để phục vụ monitoring và debugging.

### 2.2. Đồng bộ hóa và quản lý trạng thái

Một trong những thách thức lớn nhất trong multiplayer game là đảm bảo consistency của game state giữa server và tất cả clients. Hệ thống này áp dụng authoritative server model, nghĩa là server là single source of truth cho mọi game state và clients chỉ render state mà server broadcast.

**State Synchronization Flow**

Khi một client thực hiện action (vẽ, chat, đoán từ), client sẽ gửi message đến server thông qua TCP socket. Server nhận message, validate action (ví dụ: chỉ người đang vẽ mới được gửi stroke, chỉ người đang đoán mới được submit guess), update internal state nếu hợp lệ và broadcast update đến tất cả affected clients. Clients nhận broadcast và update local state để reflect change. Cách tiếp cận này đảm bảo rằng không có conflict state vì mọi decision đều được centralize tại server.

Đối với drawing synchronization, khi drawer mouse-drag trên canvas, client sẽ gửi liên tục các MSG_STROKE messages chứa line segments (x1, y1, x2, y2, color, thickness). Server không validate tọa độ hay kiểm tra collision, mà đơn giản broadcast message đến tất cả players khác trong room (exclude drawer để tránh echo). Mỗi client receiving stroke sẽ append vào local strokes array và render lại toàn bộ canvas. Thiết kế này trade off một chút server CPU (vì không optimize drawing) để đổi lấy simplicity và robustness.

**Concurrency Control**

Server sử dụng pthread_mutex để protect các shared data structures khỏi race conditions. Cụ thể, matchmaking_mutex protect việc truy cập vào rooms array khi create/join/leave rooms, stats_mutex protect việc read/write player statistics file, và các room-specific mutexes có thể được thêm nếu cần. Tuy nhiên, vì server sử dụng single-threaded event loop với select() cho I/O operations, phần lớn game logic processing không cần locking vì chúng run sequentially trong main thread. Chỉ có timer thread cần synchronized access khi update timers.

**Player Session Management**

Khi player connect và register, server assign một unique player_id (incremental counter) và store Player struct trong global players array. Player_id này được sử dụng làm identifier trong tất cả messages và game state. Khi player disconnect (detected qua recv() returning 0 hoặc negative), server remove player từ array và notify room về sự kiện này. Room sẽ adjust game state accordingly: nếu drawer disconnect thì end round sớm, nếu guesser disconnect thì chỉ update player count và check xem còn đủ người chơi không.

Hệ thống cũng implement connection monitoring thông qua PING/PONG mechanism. Server định kỳ (có thể qua timer thread hoặc khi idle) gửi MSG_PING đến clients. Client phải respond với MSG_PONG trong vòng 15 giây, nếu không server sẽ consider connection lost và disconnect player. Client side cũng track thời gian của last received PONG và hiển thị "Connection Lost" banner nếu quá 15 giây không nhận được PONG. Cách tiếp cận này detect các stale connections do network issues mà TCP protocol không tự phát hiện được (ví dụ: cable unplugged, WiFi signal lost).

### 2.3. Khả năng mở rộng và hiệu năng

Thiết kế hiện tại của server có khả năng handle khoảng 100 concurrent connections và 100 active rooms, đủ cho một deployment small-scale. Tuy nhiên, để scale lên hàng nghìn users, cần những cải tiến sau:

Thứ nhất, migrate từ select() sang epoll (Linux) hoặc kqueue (BSD/macOS) để giảm overhead khi số lượng file descriptors tăng lên. Select() có complexity O(n) vì phải iterate qua tất cả fds mỗi lần call, trong khi epoll chỉ return các fds đã ready với complexity O(1).

Thứ hai, implement thread pool để parallelize message processing. Hiện tại server process messages sequentially trong main thread, limiting throughput. Với thread pool, mỗi incoming message có thể được dispatch đến một worker thread để process, increasing overall throughput đáng kể.

Thứ ba, migrate sang non-blocking I/O cho tất cả send operations. Hiện tại send() có thể block nếu TCP send buffer đầy, causing server stall. Với non-blocking sends và write buffers, server có thể continue processing other clients ngay cả khi một client slow to receive.

Cuối cùng, consider sharding rooms across multiple server processes hoặc machines. Mỗi server instance có thể responsible for một subset của rooms, và một separate matchmaking service route players đến appropriate server. Điều này cho phép horizontal scaling beyond single-machine limits.

---

## III. THIẾT KẾ GIAO THỨC TẦNG ỨNG DỤNG

### 3.1. Lựa chọn transport protocol

Hệ thống sử dụng TCP (Transmission Control Protocol) làm transport layer protocol chính cho tất cả game communication. Quyết định này được đưa ra dựa trên phân tích các yêu cầu của application và trade-offs giữa TCP và UDP.

TCP là connection-oriented protocol cung cấp reliable, ordered delivery của byte stream. Khi application gửi data qua TCP socket, protocol stack đảm bảo rằng data sẽ được deliver đến receiver đúng thứ tự, không bị mất mát và không bị duplicate. Điều này đạt được thông qua sequence numbers, acknowledgments và automatic retransmission. TCP cũng implement flow control (receiver advertises window size) và congestion control (sender adjusts sending rate based on network conditions) để tránh overwhelm receiver hoặc congest network.

Đối với game của chúng ta, reliable delivery là critical requirement. Các message như player registration, room join/leave, chat messages và quan trọng nhất là guess submissions phải được deliver chính xác. Nếu một guess message bị lost, player sẽ không được score và game state sẽ inconsistent. Tương tự, nếu chat message bị lost, players sẽ miss communication. Việc implement custom reliability layer trên UDP sẽ tốn effort đáng kể và dễ introduce bugs.

Ordered delivery cũng quan trọng cho một số loại messages. Ví dụ, khi server broadcast MSG_ROUND_END tiếp theo là MSG_ROUND_START cho round mới, clients phải receive theo đúng thứ tự này. Nếu đảo ngược, client có thể render sai UI state. Với TCP, thứ tự được đảm bảo automatically.

Drawing data (strokes) là loại data duy nhất có thể tolerate một chút packet loss trong thời gian ngắn. Nếu một stroke message bị delay hoặc lost, drawing vẫn roughly correct vì các stroke tiếp theo sẽ fill in. Tuy nhiên, với modern networks having low packet loss rates và việc TCP retransmissions thường xảy ra nhanh chóng (vài milliseconds), overhead này là acceptable. Hơn nữa, việc sử dụng cùng một protocol cho tất cả traffic simplifies architecture và debugging.

### 3.2. Message framing protocol

Vì TCP là stream-oriented protocol, nó không preserve message boundaries. Khi application gọi send() nhiều lần, receiver có thể nhận tất cả data trong một recv() call hoặc ngược lại, một send() lớn có thể được split thành nhiều recv() calls. Application layer phải implement message framing để delimit messages.

Hệ thống của chúng ta sử dụng length-prefix framing, một phương pháp phổ biến và efficient. Mỗi message bắt đầu bằng 4 bytes biểu diễn chiều dài của payload (JSON string) theo big-endian byte order, followed by payload itself. Big-endian (network byte order) được chọn vì đây là convention standard trong network protocols, ensuring interoperability giữa các machines có byte order khác nhau.

**Sending Process:**

Khi client hoặc server muốn send message, process như sau:

Đầu tiên, message được construct dưới dạng JSON object với hai fields chính: "type" (integer message type) và "data" (object chứa message-specific fields). Ví dụ, một chat message có dạng: `{"type": 14, "data": {"message": "hello"}}`. JSON format được chọn vì human-readable (dễ debug), flexible (dễ extend với fields mới) và có sẵn parsers trong mọi ngôn ngữ.

Tiếp theo, JSON object được serialize thành string và calculate chiều dài của string (số bytes). Chiều dài này được convert sang network byte order bằng htonl() function (host to network long), producing 4-byte prefix. Prefix và JSON string được concatenate thành một buffer duy nhất, sau đó gọi send() system call một lần để gửi toàn bộ.

Sử dụng single send() call cho cả prefix và payload là optimal practice, reducing system call overhead và improving batching. Tuy nhiên, application phải prepare cho possibility rằng send() có thể chỉ send một phần của buffer (partial send). Trong trường hợp này, application cần loop và gọi send() lại cho remaining data, hoặc better là switch sang non-blocking I/O với send buffers.

**Receiving Process:**

Receiving side phức tạp hơn vì phải handle partial receives và buffer management. Mỗi connection maintain một receive buffer (ví dụ: 4096 bytes) và một offset pointer indicating bao nhiêu valid data trong buffer.

Khi recv() return với new data, data được append vào buffer tại offset position và offset được increment. Sau đó, parser attempt to extract complete messages từ buffer. Parser loop như sau:

Đầu tiên, check xem buffer có ít nhất 4 bytes không (để đọc length prefix). Nếu không đủ, break loop và wait for more data. Nếu đủ, read 4 bytes đầu và convert từ network byte order sang host byte order bằng ntohl() function, getting message length.

Tiếp theo, check xem buffer có đủ 4 + message_length bytes không (entire message). Nếu không đủ, break loop và wait. Nếu đủ, extract message_length bytes sau prefix làm JSON string, parse JSON và dispatch đến message handler. Sau khi process message, advance một pointer để skip qua message đã process.

Cuối cùng, sau khi extract tất cả complete messages, move remaining data (incomplete message) về đầu buffer và adjust offset. Điều này ensures buffer không overflow và luôn có space cho new data.

### 3.3. Message types và định nghĩa

Protocol định nghĩa 29 message types cho TCP communication, được organize thành các categories:

**Connection Management Messages (0-1):**

MSG_PING và MSG_PONG implement keepalive mechanism. Server có thể send PING để verify client còn alive, client phải respond với PONG. Nếu không receive PONG sau timeout, connection considered lost. Messages này không có payload data, chỉ cần type field.

**Authentication & Registration Messages (2-3):**

MSG_REGISTER được client gửi sau khi connect, chứa username trong data field. Server validate username (không trùng, không chứa ký tự đặc biệt), assign player_id và respond với MSG_REGISTER_ACK chứa player_id và username. Player_id là unique identifier được sử dụng trong tất cả subsequent communication.

**Room Management Messages (4-9):**

MSG_JOIN_ROOM cho auto matchmaking (room_id = 0) hoặc join specific room. MSG_CREATE_ROOM request tạo private room. Server respond với MSG_ROOM_CREATED (chứa room_code), MSG_ROOM_JOINED (chứa room info và player list), MSG_ROOM_FULL hoặc MSG_ROOM_NOT_FOUND tùy outcome.

**Game Flow Messages (10-21):**

MSG_GAME_START được broadcast khi game bắt đầu, chứa round number, total rounds, player list và word mask. MSG_YOUR_TURN inform player là drawer cho round này. MSG_WORD_TO_DRAW chỉ gửi cho drawer, chứa actual word to draw. MSG_ROUND_START broadcast khi start round mới, MSG_ROUND_END khi round ends.

MSG_CHAT được client gửi, server validate và broadcast MSG_CHAT_BROADCAST. Nếu message match current word, server broadcast MSG_GUESS_CORRECT với player info và score thay vì broadcast chat. MSG_TIMER_UPDATE broadcast mỗi giây với remaining time. MSG_COUNTDOWN_UPDATE broadcast trong countdown phase trước khi game starts.

MSG_GAME_END broadcast khi tất cả rounds finished, chứa final player rankings sorted by score. MSG_PLAYER_JOIN và MSG_PLAYER_LEAVE notify khi players join/leave room.

**Host Control Messages (27-28):**

MSG_HOST_START_GAME cho phép host của private room start game sớm (bypass 15s countdown). MSG_HOST_KICK_PLAYER cho phép host remove player từ room. Server validate rằng sender là host trước khi execute actions.

**Drawing Messages (100-102):**

MSG_STROKE (type 100) chứa drawing data: x1, y1, x2, y2 (line segment coordinates), color (palette index 0-9) và thickness (2-20 pixels). MSG_CLEAR_CANVAS (type 101) request xóa toàn bộ canvas. Messages này technically là UDP_STROKE và UDP_CLEAR_CANVAS trong code (historical naming) nhưng actually transmitted qua TCP.

### 3.4. JSON payload structure

Mỗi message có structure: `{"type": <number>, "data": <object>}`. Data object varies theo message type. Ví dụ:

**Registration Request:**
```json
{
  "type": 2,
  "data": {
    "username": "Alice"
  }
}
```

**Registration Acknowledgment:**
```json
{
  "type": 3,
  "data": {
    "player_id": 123,
    "username": "Alice"
  }
}
```

**Stroke Drawing:**
```json
{
  "type": 100,
  "data": {
    "x1": 150.5,
    "y1": 200.3,
    "x2": 151.2,
    "y2": 201.8,
    "color": 1,
    "thickness": 5
  }
}
```

**Guess Correct Notification:**
```json
{
  "type": 16,
  "data": {
    "player_id": 456,
    "username": "Bob",
    "score": 850,
    "total_correct": 3
  }
}
```

Server sử dụng utility functions trong json.c để create, parse và extract fields từ JSON strings. Các functions này wrap basic string operations và sscanf/sprintf để avoid dependency on heavy JSON libraries như cJSON, keeping server lightweight.

### 3.5. Error handling và reliability

Protocol implement nhiều lớp error handling:

**Transport Layer:** TCP đảm bảo reliable delivery thông qua acknowledgments và retransmissions. Application không cần handle packet loss hay reordering.

**Framing Layer:** Length-prefix protocol ensure message boundaries are preserved. Nếu receive buffer overflow (message quá lớn), connection bị close để avoid corruption.

**Application Layer:** Server validate tất cả incoming messages: check message type hợp lệ, validate JSON structure, verify player có permission để perform action, check game state allows action. Nếu validation fails, server có thể send MSG_ERROR response hoặc simply ignore message.

**Connection Monitoring:** PING/PONG mechanism detect stale connections do network issues. Nếu client không respond trong 15 seconds, server proactively close connection và cleanup resources.

**Graceful Degradation:** Nếu server busy hoặc overloaded, nó có thể reject new connections thay vì accept và crash. Clients receive connection refused error và có thể retry sau exponential backoff.

---

## IV. NỀN TẢNG VÀ THƯ VIỆN XÂY DỰNG ỨNG DỤNG

### 4.1. Server backend technologies

**Programming Language: C11**

Server được implement hoàn toàn bằng C programming language với C11 standard. Lựa chọn C là deliberate decision dựa trên several factors. Thứ nhất, C cung cấp direct access đến system calls và low-level operations, critical cho network programming. Việc gọi socket(), bind(), listen(), accept(), send(), recv() trực tiếp mà không qua abstraction layers cho phép fine-grained control over network behavior và performance optimization.

Thứ hai, C có minimal runtime overhead so với higher-level languages. Không có garbage collection pauses, không có virtual machine, không có hidden allocations. Mỗi allocation và deallocation là explicit, allowing precise memory management. Điều này quan trọng cho server phải maintain state của hàng trăm concurrent connections và có thể process thousands of messages per second.

Thứ ba, C code có excellent portability across Unix-like systems (Linux, macOS, BSD). Sử dụng POSIX APIs đảm bảo code compile và run trên tất cả platforms without modification. Standard headers như sys/socket.h, netinet/in.h, pthread.h là standard trên mọi Unix systems.

**POSIX Threads (pthreads)**

Server sử dụng POSIX threads cho concurrent execution. Cụ thể, một thread chạy TCP server loop với select() để handle client connections và messages, và một timer thread riêng chạy mỗi giây để update game timers và countdown. Threads synchronize qua pthread_mutex_t khi access shared data structures như rooms array và stats file.

Pthreads API được chọn thay vì C11 threads (threads.h) vì wider support và mature ecosystem. Pthreads cung cấp đầy đủ primitives: pthread_create() để spawn threads, pthread_join() để wait for completion, pthread_mutex_lock/unlock() cho mutual exclusion, và pthread_cond_wait/signal() cho condition variables (mặc dù chưa sử dụng trong current implementation).

**Berkeley Sockets API**

Networking layer sử dụng Berkeley Sockets API, interface chuẩn cho network programming trên Unix systems. API này được design trong những năm 1980s tại UC Berkeley như part of BSD Unix và since then đã become de facto standard.

Key functions used:
- `socket()`: Create socket descriptor
- `setsockopt()`: Set socket options (e.g., SO_REUSEADDR)
- `bind()`: Bind socket to address/port
- `listen()`: Mark socket as passive (listening for connections)
- `accept()`: Accept incoming connection (blocking)
- `connect()`: Initiate connection to server (client-side)
- `send()`/`recv()`: Send/receive data on connected socket
- `close()`: Close socket descriptor

Server sử dụng `select()` system call cho I/O multiplexing. Select() allows server monitor multiple file descriptors (sockets) simultaneously và block until ít nhất một fd ready for I/O. Khi select() returns, server can determine which fds are ready và process them accordingly. Approach này efficient hơn nhiều so với spawning một thread per connection hoặc busy-waiting on each socket.

**Standard C Library**

Server rely on standard C library (libc) cho các utilities: string operations (string.h), memory management (stdlib.h), input/output (stdio.h), time operations (time.h). Không sử dụng external dependencies như JSON libraries để keep binary size small và avoid dependency issues. Instead, implement simple JSON parsing/generation functions sử dụng string operations.

### 4.2. Client technologies

**Python 3**

Client application được develop bằng Python 3 (version 3.8 trở lên). Python được chọn vì rapid development capability, extensive libraries ecosystem và excellent integration với C code thông qua ctypes. Trong context của game client nơi performance không critical như server và developer productivity is valued, Python là perfect choice.

Python's syntax và semantics allow express complex game logic một cách concise và readable. Object-oriented features như classes và inheritance giúp organize code thành maintainable modules. Dynamic typing reduces boilerplate code và speeds up iteration during development.

**Pygame 2.x**

Pygame là thư viện multimedia chuyên dụng cho game development trong Python, built on top of SDL (Simple DirectMedia Layer) library. Pygame cung cấp modules cho graphics rendering, sound playback, input handling và timing.

Key Pygame features used:
- `pygame.display`: Manage display window, screen surfaces, và vsync
- `pygame.draw`: Drawing primitives như lines, circles, rectangles
- `pygame.font`: Text rendering with TrueType fonts
- `pygame.event`: Event queue và event handling (mouse, keyboard, window events)
- `pygame.time.Clock`: Frame rate control và delta timing
- `pygame.mouse`: Mouse position và button state queries
- `pygame.Surface`: 2D image representations, blitting operations

Pygame uses hardware acceleration khi available thông qua SDL2 backend, enabling smooth rendering even khi draw nhiều strokes. Frame rate được maintain ở 60 FPS với Clock.tick(60), ensuring consistent update timing.

**ctypes Foreign Function Interface**

ctypes là Python standard library module cho calling C functions từ shared libraries. ctypes cho phép Python code load shared library (.dylib trên macOS, .so trên Linux), declare function signatures, và call C functions như native Python functions.

Example ctypes usage trong project:
```python
import ctypes
from ctypes import c_int, c_char_p, POINTER

# Load shared library
lib = ctypes.CDLL('./libscribble_client.dylib')

# Declare function signatures
lib.network_connect.argtypes = [c_char_p, c_int]
lib.network_connect.restype = c_int

lib.network_send_tcp.argtypes = [c_char_p, c_int]
lib.network_send_tcp.restype = c_int

# Call C functions
result = lib.network_connect(b"127.0.0.1", 9090)
if result == 0:
    message = b'{"type":2,"data":{"username":"Alice"}}'
    lib.network_send_tcp(message, len(message))
```

ctypes handles conversion giữa Python types và C types automatically. Python strings phải encode thành bytes (UTF-8) trước khi pass vào C functions expecting char pointers. Return values được convert từ C types về Python types.

Performance overhead của ctypes calls là minimal (microseconds per call), acceptable cho game loop running at 60 FPS. Mỗi frame client chỉ gọi vài ctypes functions để send/receive messages.

### 4.3. Networking library design

**Shared Library Compilation**

C networking library được compile thành shared library (dynamic library) thay vì static linking. Trên macOS, compilation command:
```bash
gcc -dynamiclib -o libscribble_client.dylib network.c \
    -std=c11 -Wall -Wextra -O2
```

Trên Linux:
```bash
gcc -shared -fPIC -o libscribble_client.so network.c \
    -std=c11 -Wall -Wextra -O2 -lpthread
```

Shared library approach has several advantages:
- Library code có thể được reused bởi multiple applications
- Updates đến library không require recompiling applications
- Memory efficient khi multiple processes load same library
- Allows mixing languages (C library called từ Python, Java, etc.)

**API Design Principles**

Library API được design theo principles sau:

Simplicity: Expose minimal set of functions với clear purpose. Only four main functions: connect, send, receive, disconnect. Hiding complexity như threading, buffering, framing inside implementation.

Error Handling: Functions return integer status codes (0 for success, negative for errors). Additional error details available via network_get_last_error() function returning string message.

Resource Management: Library manages all internal resources (sockets, threads, buffers). Caller chỉ cần call connect() để initialize và disconnect() để cleanup, không cần worry about memory leaks hay resource cleanup.

Thread Safety: Receive operations run trong background thread, but API ensures thread-safe access thông qua mutexes protecting shared state. Caller có thể safely gọi send() từ main thread while receive thread active.

### 4.4. Build system

**Makefile Structure**

Project sử dụng GNU Make cho build automation. Makefile định nghĩa targets để compile server, client library và run applications.

Server build process:
1. Compile mỗi .c file thành .o object file với gcc
2. Link tất cả object files thành final executable
3. Enable compiler warnings (-Wall -Wextra) để catch potential bugs
4. Use optimization level -O2 for production builds

Client library build:
1. Compile network.c với -fPIC flag (position-independent code) cho shared library
2. Link thành .dylib (macOS) hoặc .so (Linux)
3. Ensure exported symbols visible với proper visibility attributes

Makefile cũng định nghĩa phony targets như `clean` (remove build artifacts), `run-server` (start server), `run-client` (start Python client với arguments), making development workflow efficient.

**Cross-Platform Considerations**

Code được viết để portable across Unix-like systems nhưng có minor differences:
- macOS sử dụng .dylib extension cho shared libraries, Linux sử dụng .so
- Some socket options khác nhau (e.g., SO_NOSIGPIPE trên macOS)
- Endianness handling với htonl/ntohl ensures compatibility between architectures
- Path separators trong Python code handled với os.path module

Testing on multiple platforms ensures compatibility và catches platform-specific issues early.

---

## V. CƠ CHẾ KIỂM SOÁT NHIỀU MÁY KHÁCH CỦA SERVER

### 5.1. I/O multiplexing với select()

**Cơ chế kiểm soát nhiều máy khách của server**

Một trong những thách thức chính trong thiết kế server mạng là xử lý hiệu quả nhiều kết nối đồng thời. Mỗi client duy trì một kết nối socket TCP đến server, và server phải đồng thời giám sát tất cả các sockets để kiểm tra dữ liệu đến, đồng thời chấp nhận các kết nối mới. Phương pháp truyền thống là spawn một thread cho mỗi kết nối sẽ nhanh chóng trở nên không khả thi khi số lượng clients tăng lên do overhead bộ nhớ và chi phí context switching.

Hệ thống này sử dụng I/O multiplexing với `select()` system call để giải quyết thách thức này. Select() là một trong những cơ chế multiplexing lâu đời nhất, được hỗ trợ trên tất cả các hệ thống Unix-like và cung cấp giải pháp portable.

**Cơ chế Select()**

Lệnh gọi hệ thống select() cho phép server giám sát nhiều file descriptors đồng thời và block cho đến khi ít nhất một fd sẵn sàng cho hoạt động I/O (đọc, ghi hoặc có exception). Chữ ký API:

```c
int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);
```

Server sử dụng select() như sau:

Trước mỗi lần lặp của vòng lặp chính, server tạo một fd_set (tập các file descriptors) và thêm tất cả các sockets cần giám sát. Cụ thể, server thêm listening socket (để chấp nhận các kết nối mới) và tất cả các client sockets (để nhận dữ liệu):

```c
fd_set read_fds;
FD_ZERO(&read_fds);  // Xóa tập hợp
FD_SET(server_fd, &read_fds);  // Thêm listening socket

int max_fd = server_fd;
for (int i = 0; i < player_count; i++) {
    FD_SET(players[i].fd, &read_fds);
    if (players[i].fd > max_fd) max_fd = players[i].fd;
}
```

Sau đó, server gọi select() với timeout (ví dụ: 1 giây) để tránh blocking vô thời hạn:

```c
struct timeval timeout;
timeout.tv_sec = 1;
timeout.tv_usec = 0;

int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
```

Select() blocks cho đến khi ít nhất một fd sẵn sàng hoặc timeout hết. Khi trả về, tập readfds được sửa đổi để chỉ ra fd nào sẵn sàng. Server sau đó kiểm tra từng fd:

```c
if (FD_ISSET(server_fd, &read_fds)) {
    // Listening socket sẵn sàng - chấp nhận kết nối mới
    int client_fd = accept(server_fd, ...);
    // Thêm vào mảng players
}

for (int i = 0; i < player_count; i++) {
    if (FD_ISSET(players[i].fd, &read_fds)) {
        // Client socket sẵn sàng - nhận dữ liệu
        int bytes = recv(players[i].fd, buffer, size, 0);
        // Xử lý message
    }
}
```

**Advantages and Limitations**

Select() approach has several advantages:
- Single-threaded: No need for complex thread synchronization
- Efficient: Server only wakes up khi có actual I/O activity
- Portable: Works on all Unix systems without modification
- Simple: Straightforward logic dễ understand và debug

Tuy nhiên, select() có limitations:
- O(n) complexity: Must iterate qua all fds để check which are ready
- fd_set size limit: Maximum 1024 file descriptors trên nhiều systems
- Must reconstruct fd_set before mỗi call vì kernel modifies it

Cho current scale (100 clients), select() performs adequately. For larger scales, server should migrate sang epoll (Linux) hoặc kqueue (BSD/macOS) which have O(1) complexity và no hardcoded limits.

### 5.2. Quản lý vòng đời kết nối

**Thiết lập kết nối**

Khi client gọi network_connect() với IP và cổng của server, thư viện C tạo socket TCP và khởi tạo bắt tay ba chiều (three-way handshake). Socket listening của server nhận yêu cầu kết nối, kernel hoàn thành bắt tay tự động, và lệnh gọi accept() trả về descriptor socket mới được kết nối với client.

Server bọc descriptor socket trong struct Player, khởi tạo các trường (fd, username rỗng, player_id = 0, state = PLAYER_DISCONNECTED ban đầu) và thêm vào mảng toàn cục players. Chỉ số mảng phục vụ như định danh nội bộ, trong khi player_id được gán sau khi đăng ký.

**Nhận dữ liệu và phân tích cú pháp tin nhắn**

Khi select() cho biết socket client sẵn sàng, server gọi recv() để đọc dữ liệu vào bộ đệm nhận:
```c
int bytes_read = recv(players[i].fd, 
                     players[i].recv_buffer + players[i].recv_buffer_len,
                     space_available, 0);
```

Dữ liệu được nối thêm vào nội dung bộ đệm hiện tại vì TCP có thể chuyển các tin nhắn một cách từng phần. Độ dài bộ đệm được theo dõi để biết nơi ghi dữ liệu mới.

Sau khi nhận, server cố gắng phân tích cú pháp các tin nhắn hoàn chỉnh từ bộ đệm. Trình phân tích cú pháp đọc tiền tố 4 byte độ dài, kiểm tra xem tin nhắn đầy đủ có sẵn không, trích xuất tin nhắn và loại bỏ khỏi bộ đệm. Quá trình lặp lại cho tất cả các tin nhắn hoàn chỉnh, để lại tin nhắn một phần trong bộ đệm cho recv() tiếp theo.

**Gửi tin nhắn**

Sau khi phân tích loại tin nhắn và dữ liệu JSON, server gửi đến hàm xử lý thích hợp dựa trên loại tin nhắn:
```c
switch (msg_type) {
    case MSG_REGISTER:
        handle_register(player, json_data);
        break;
    case MSG_JOIN_ROOM:
        handle_join_room(player, json_data);
        break;
    case MSG_CHAT:
        handle_chat(player, json_data);
        break;
    // ... các loại tin nhắn khác
}
```

Mỗi hàm xử lý xác thực yêu cầu (kiểm tra trạng thái player, trạng thái phòng, quyền), cập nhật trạng thái trò chơi nếu hợp lệ và tạo phản hồi. Phản hồi được gửi ngay lập tức hoặc phát sóng đến nhiều người chơi.

**Kết thúc kết nối**

Kết nối có thể kết thúc vì nhiều lý do:
- Client gọi disconnect một cách rõ ràng: Client gửi gói FIN, recv() của server trả về 0
- Lỗi mạng: Timeout kết nối, recv() của server trả về -1 với ETIMEDOUT
- Server khởi tạo ngắt kết nối: Server gọi close(fd) khi loại bỏ người chơi hoặc phát hiện kết nối cũ

Khi phát hiện ngắt kết nối, server gọi hàm handle_disconnect():
```c
void handle_disconnect(Player* player) {
    Room* room = get_player_room(player);
    if (room) {
        // Thông báo cho người chơi khác
        char info[256];
        snprintf(info, sizeof(info), "{\"player_id\":%u,\"username\":\"%s\"}",
                 player->player_id, player->username);
        broadcast_to_room(room, MSG_PLAYER_LEAVE, info, player);
        
        // Loại bỏ khỏi phòng
        leave_room(player);
    }
    // Socket đã được đóng bởi lệnh gọi trước
}
```

Player được loại bỏ khỏi phòng, những người chơi khác được thông báo, và trạng thái trò chơi được điều chỉnh (ví dụ: kết thúc vòng nếu người vẽ ngắt kết nối). Descriptor socket được đóng và struct Player bị xóa khỏi mảng.

### 5.3. Quản lý phòng chơi đồng thời

Server có thể lưu trữ nhiều phòng chơi đồng thời, mỗi phòng là một phiên chơi độc lập. Các phòng được lưu trữ trong mảng toàn cục gồm 100 struct Room.

**Máy trạng thái phòng chơi**

Mỗi phòng có máy trạng thái với ba trạng thái:
- ROOM_WAITING: Phòng được tạo, chờ người chơi và bộ đếm ngược
- ROOM_PLAYING: Trò chơi hoạt động, xử lý các vòng
- ROOM_ENDED: Trò chơi kết thúc, người chơi có thể rời đi

Chuyển đổi trạng thái:
- WAITING → PLAYING: Khi bộ đếm ngược đạt 0 hoặc host khởi động sớm
- PLAYING → ENDED: Khi hoàn thành tất cả các vòng
- ENDED → WAITING: Phòng được đặt lại cho trò chơi mới (hiện tại không triển khai, phòng trở thành không hoạt động)

**Cách ly phòng chơi**

Mỗi phòng duy trì trạng thái riêng: danh sách người chơi, người vẽ hiện tại, từ, số vòng, bộ hẹn giờ, strokes canvas. Các hoạt động trên một phòng không ảnh hưởng đến các phòng khác. Khi server phát sóng tin nhắn, nó chỉ gửi đến những người chơi trong cùng một phòng:

```c
void broadcast_to_room(Room* room, MessageType type, 
                       const char* data, Player* exclude) {
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i] && room->players[i] != exclude) {
            send_tcp_message(room->players[i]->fd, type, data);
        }
    }
}
```

Tham số exclude cho phép bỏ qua người gửi khi phát sóng (ví dụ: không gửi stroke lại cho người vẽ).

**Vòng đời phòng chơi**

Vòng đời phòng chơi:
1. Được tạo: Khi người chơi đầu tiên tham gia thông qua ghép cặp hoặc tạo phòng riêng tư
2. Người chơi tham gia: Người chơi được thêm vào phòng, player_count tăng
3. Bộ đếm ngược: Khi có 2+ người chơi, bộ đếm ngược 15 giây bắt đầu
4. Bắt đầu trò chơi: Bộ đếm ngược hết, total_rounds được đặt thành player_count, vòng đầu tiên bắt đầu
5. Các vòng: Người chơi lần lượt vẽ, những người khác đoán, bộ hẹn giờ chạy, vòng kết thúc
6. Kết thúc trò chơi: Hoàn thành tất cả các vòng, điểm số cuối cùng được phát sóng, phòng chuyển sang trạng thái ENDED
7. Dọn dẹp: Người chơi rời đi, phòng trở thành không hoạt động (player_count = 0)

Các phòng không hoạt động có thể được tái sử dụng cho các trò chơi mới. Server sử dụng thuật toán first-fit để tìm phòng có sẵn khi người chơi tham gia ghép cặp.

### 5.4. An toàn luồng và đồng bộ hóa

Server sử dụng khóa tối thiểu vì vòng lặp trò chơi chính là đơn luồng. Tuy nhiên, luồng bộ hẹn giờ chạy đồng thời và truy cập trạng thái phòng chung, yêu cầu đồng bộ hóa.

**Mutex Ghép cặp**

Global matchmaking_mutex bảo vệ mảng rooms khi tạo/tìm/lặp qua phòng. Luồng bộ hẹn giờ khóa mutex khi lặp qua phòng để cập nhật bộ hẹn giờ, ngăn chặn điều kiện đua với luồng chính tạo/hủy phòng.

```c
void iterate_active_rooms(void (*callback)(Room*)) {
    pthread_mutex_lock(&matchmaking_mutex);
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].player_count > 0 && 
            (rooms[i].state == ROOM_PLAYING || rooms[i].state == ROOM_WAITING)) {
            callback(&rooms[i]);
        }
    }
    pthread_mutex_unlock(&matchmaking_mutex);
}
```

**Mutex Thống kê**

stats_mutex bảo vệ tệp player_stats.txt khi đọc/ghi thống kê người chơi. Nhiều luồng (luồng chính lưu thống kê, có thể nhiều trò chơi kết thúc đồng thời) có thể làm hỏng tệp nếu không khóa.

**Kỹ thuật không khóa**

Trong vòng lặp chính, server xử lý các tin nhắn tuần tự, loại bỏ nhu cầu khóa. Trạng thái của mỗi phòng chỉ được truy cập bởi luồng xử lý các tin nhắn từ những người chơi trong phòng đó (hiệu quả là đơn luồng trên mỗi phòng).

Luồng bộ hẹn giờ chỉ đọc trạng thái phòng để tính toán thời gian còn lại và ghi các trường hạn chế (time_remaining). Được thiết kế để chịu những sự không nhất quán nhỏ (ví dụ: cập nhật bộ hẹn giờ bị trễ một giây không quan trọng).


### 5.5. Resource limits và scalability

**Memory Management**

Server allocates fixed-size arrays for players (100) và rooms (100), totaling approximately:
- Players array: 100 * sizeof(Player) ≈ 100 * 4KB ≈ 400KB
- Rooms array: 100 * sizeof(Room) ≈ 100 * 50KB ≈ 5MB
- Total: ~5.5MB static allocation

Dynamic allocations minimal: JSON strings (freed immediately after use), word list (loaded once at startup). No per-message allocations trong hot path, avoiding fragmentation và improving cache locality.

**CPU Usage**

Server CPU usage scales linearly với số lượng active connections và message rate. Với 100 clients sending messages moderately (chat, strokes occasionally), modern CPU easily handles load. Bottleneck likely network bandwidth trước khi CPU saturation.

Select() overhead is O(n) với số fds, but for n=100, overhead negligible (microseconds). Message parsing và JSON manipulation are most CPU-intensive operations, but optimized với simple string operations rather than heavyweight libraries.

**Network Bandwidth**

Assuming:
- Average message size: 200 bytes (JSON overhead)
- Drawing: 10 strokes/second per drawer = 2KB/s
- Broadcast factor: 4 players per room = 8KB/s per room
- 25 active rooms = 200KB/s = 1.6 Mbps

Even with 100% utilization, bandwidth requirements modest by modern standards. Local network (Gigabit Ethernet) hoặc Internet connection (broadband) easily handles load.

**File Descriptors**

Each client connection consumes một file descriptor. OS limits number of open fds per process (ulimit -n, typically 1024-65536). With 100 clients plus listening socket, server uses ~101 fds, well within limits.

**Scalability Improvements**

To scale beyond 100 concurrent clients:

Migrate từ select() sang epoll/kqueue để improve from O(n) to O(1) fd monitoring. Implement thread pool để parallelize message processing. Consider sharding rooms across multiple server instances với load balancer. Optimize JSON parsing với specialized library như cJSON. Implement connection pooling và reuse buffers để reduce allocations.

With these optimizations, server có thể handle thousands of concurrent connections trên single machine.

---

## VI. KẾT LUẬN

### 6.1. Tổng kết

Dự án đã successfully implement một hệ thống multiplayer game hoàn chỉnh sử dụng Berkeley Sockets API và TCP protocol. Hệ thống thể hiện các khái niệm cơ bản của network programming: socket creation và binding, thiết lập kết nối, truyền dữ liệu với proper framing, I/O multiplexing, và kết thúc kết nối một cách graceful.

Kiến trúc được thiết kế theo phương pháp layered với sự tách biệt rõ ràng về trách nhiệm: presentation layer (Pygame client) xử lý UI và user input, networking layer (C shared library) trừu tượng hóa các socket operations, và business logic layer (C server) cài đặt các luật chơi và quản lý state. Cách tiếp cận này thúc đẩy code reusability, testability và maintainability.

Thiết kế protocol cân bằng giữa simplicity và extensibility. Length-prefix framing với JSON payloads cung cấp format dễ đọc dễ debug đồng thời vẫn đủ efficient cho real-time game. Các message types được thiết kế cẩn thận để bao quát tất cả các kịch bản trò chơi từ đăng ký đến kết thúc trò chơi.

Cài đặt server thể hiện hiệu quả sử dụng tài nguyên với single-threaded event loop sử dụng select() multiplexing. Cách tiếp cận này cho phép server xử lý hàng chục kết nối đồng thời với CPU và memory overhead tối thiểu. Thread safety được đảm bảo thông qua locking cẩn thận của các shared resources.

### 6.2. Bài học kinh nghiệm

Qua quá trình develop project, several lessons learned:

**TCP vs UDP Trade-offs:** Initial consideration was using UDP cho drawing data để reduce latency. Tuy nhiên, complexity của implementing reliability layer (sequence numbers, acks, retransmissions) outweighed benefits. TCP's built-in reliability sufficient cho game's requirements, và performance acceptable với modern networks.

**Message Framing is Critical:** Early versions had bugs với partial message receives. Learning proper framing techniques với length-prefix và careful buffer management essential cho robust network applications.

**Error Handling:** Network programming is inherently error-prone do unpredictable network conditions. Defensive programming với extensive error checking, logging và graceful degradation prevents crashes và helps debugging.

**Testing Challenges:** Testing multiplayer systems requires simulating multiple clients, various network conditions (packet loss, delays, disconnections) và race conditions. Automated testing scripts và manual testing với multiple machines necessary.

**Performance Optimization:** Premature optimization avoided. Initial focus on correctness và functionality, sau đó profile để identify bottlenecks. Many assumed bottlenecks (JSON parsing, select() overhead) proved non-issues trong practice.

### 6.3. Hướng phát triển

Potential improvements và extensions:

**Enhanced Features:** Thêm spectator mode allowing users watch games without playing, implement undo functionality cho drawing, add hint system (reveal letters gradually), support custom word lists, và implement player profiles với statistics history.

**Performance Optimizations:** Migrate sang epoll/kqueue cho better scalability, implement connection pooling, optimize JSON serialization với binary protocol hoặc specialized library, và add caching layers cho frequently accessed data.

**Security Enhancements:** Implement authentication system với password hoặc OAuth, add rate limiting để prevent spam và DoS attacks, validate all user inputs để prevent injection attacks, và encrypt sensitive data.

**Deployment:** Containerize với Docker cho easy deployment, set up CI/CD pipeline với automated testing, implement monitoring với Prometheus/Grafana, và configure load balancing cho high availability.

**Code Quality:** Increase test coverage với unit tests và integration tests, perform static analysis với tools như Valgrind và AddressSanitizer để detect memory issues, và refactor code để improve readability và reduce complexity.

Project serves as solid foundation cho these enhancements và demonstrates practical application của network programming concepts trong real-world scenarios.

---

**Tài liệu tham khảo:**

1. Stevens, W. Richard. "Unix Network Programming, Volume 1: The Sockets Networking API" (3rd Edition)
2. Beej's Guide to Network Programming - https://beej.us/guide/bgnet/
3. POSIX.1-2017 (IEEE Std 1003.1-2017) - System Interfaces
4. RFC 793 - Transmission Control Protocol (TCP)
5. Pygame Documentation - https://www.pygame.org/docs/
6. Python ctypes Documentation - https://docs.python.org/3/library/ctypes.html

---

**PHỤ LỤC**

**A. Cấu trúc thư mục dự án**
```
.
├── server/
│   ├── main.c
│   ├── protocol.h
│   ├── tcp/
│   │   ├── tcp_server.c/.h
│   │   ├── tcp_handler.c/.h
│   │   └── tcp_parser.c/.h
│   ├── game/
│   │   ├── game_logic.c/.h
│   │   ├── matchmaking.c/.h
│   │   └── stats.c/.h
│   └── utils/
│       ├── logger.c/.h
│       ├── timer.c/.h
│       └── json.c/.h
├── client_c/
│   ├── network.c/.h
│   └── Makefile
├── client_pygame/
│   ├── main.py
│   ├── network_wrapper.py
│   ├── protocol.py
│   └── resources.py
├── words.txt
├── player_stats.txt
└── Makefile
```

**B. Thông số kỹ thuật**
- TCP Port: 9090
- Maximum concurrent connections: 100
- Maximum rooms: 100
- Players per room: 2-5
- Round time: 90 seconds
- Countdown time: 15 seconds
- Message framing: 4-byte length prefix (big-endian)
- Message format: JSON
- Canvas size: 700x500 pixels
- Drawing colors: 10 (palette-based)
- Brush size range: 2-20 pixels

**C. Hướng dẫn build và chạy**

**Build server:**
```bash
cd /path/to/project
make clean
make server
```

**Build client library:**
```bash
make client-lib
```

**Run server:**
```bash
./build/scribble_server
```

**Run client:**
```bash
cd client_pygame
python3 main.py --host <server_ip> --port 9090
```

**Clean build artifacts:**
```bash
make clean
```
