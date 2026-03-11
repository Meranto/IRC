# ft_irc - Internet Relay Chat Server

*This project has been created as part of the 42 curriculum by anmerten , phoang.*

## Description

ft_irc is a fully functional IRC (Internet Relay Chat) server implementation written in C++98. This server complies with the IRC protocol and supports multiple simultaneous clients, channel management, operator privileges, and all mandatory IRC features as specified in the 42 project requirements.

### Key Features
- **Multi-client Support**: Handle multiple concurrent connections using poll()
- **User Authentication**: Secure password-based authentication system
- **Channel Management**: Create, join, and manage channels with various modes
- **Operator Commands**: Full support for channel operator privileges
- **Direct Messaging**: Private messages between users
- **Channel Modes**: invite-only (i), topic restrictions (t), passwords (k), operator privileges (o), user limits (l)

## Instructions

### Prerequisites
- C++ compiler with C++98 support (g++, clang++)
- make
- netcat (nc) for testing
- Optional: irssi, WeeChat, or HexChat for GUI testing

### Compilation

To compile the project:

```bash
make
```

Available make targets:
- `make` or `make all` - Compile the project
- `make clean` - Remove object files
- `make fclean` - Remove object files and executable
- `make re` - Recompile everything

### Execution

Run the server with:

```bash
./ircserv <port> <password>
```

**Parameters:**
- `<port>`: The port number (1-65535) on which the server will listen
- `<password>`: The connection password required for all clients

**Example:**
```bash
./ircserv 6667 SecurePass123
```

The server will display:
```
========================================
  IRC Server v1.0
  Port: 6667
========================================
[SERVER] Initializing on port 6667
[SERVER] Ready and listening!
[SERVER] Event loop started. Awaiting connections...
```

### Testing


#### Manual Testing with netcat
Use netcat directly:

```bash
nc localhost 6667
```

Then type IRC commands:
```
PASS yourpassword
NICK alice
USER alice 0 * :Alice User
JOIN #general
PRIVMSG #general :Hello everyone!
PART #general :Goodbye
QUIT :Leaving
```

#### Testing with Real IRC Clients

**sic** (recommended for terminal):
```bash
./sic -h localhost -p (port)
:NICK (nickname)
:PASS (password)
```

**HexChat** (GUI):
1. Add new network
2. Server: localhost/6667
3. Set server password
4. Connect and join channels

### Project Structure
```
ft_irc/
├── Makefile                        # Build configuration
├── README.md                       # This file
├── test_server.sh                  # Automated test script
├── manual_client.sh                # Manual testing tool
├── include/                        # Header files
│   ├── Server.hpp                  # Server class declaration
│   ├── Client.hpp                  # Client class declaration
│   └── Channel.hpp                 # Channel class declaration
└── src/                            # Implementation files
    ├── main.cpp                    # Entry point
    ├── Server.cpp                  # Server core implementation
    ├── ServerUtils.cpp             # Helper functions
    ├── ServerCommands.cpp          # Basic IRC commands
    ├── ServerOperatorCommands.cpp  # Operator commands
    ├── Client.cpp                  # Client management
    └── Channel.cpp                 # Channel management
```

## Features

### Implemented Commands

#### Connection & Registration
- **PASS** `<password>` - Authenticate with server password
- **NICK** `<nickname>` - Set or change your nickname
- **USER** `<username> 0 * <realname>` - Set username and real name

#### Channel Operations
- **JOIN** `<#channel> [key]` - Join a channel (create if doesn't exist)
- **PART** `<#channel> [message]` - Leave a channel
- **TOPIC** `<#channel> [topic]` - View or set channel topic
- **PRIVMSG** `<target> :message` - Send message to channel or user

#### Channel Operator Commands
- **KICK** `<#channel> <nick> [reason]` - Remove user from channel
- **INVITE** `<nick> <#channel>` - Invite user to channel
- **MODE** `<#channel> <modes> [params]` - Change channel modes

#### Channel Modes
- **+i** / **-i** - Set/remove invite-only mode
- **+t** / **-t** - Set/remove topic restriction to operators
- **+k** `<key>` / **-k** - Set/remove channel password
- **+o** `<nick>` / **-o** `<nick>` - Give/take operator privileges
- **+l** `<limit>` / **-l** - Set/remove user limit

#### Other Commands
- **PING** `<token>` - Server keep-alive check
- **QUIT** `[message]` - Disconnect from server

### Command Examples

```irc
# Connect and authenticate
PASS mypassword
NICK alice
USER alice 0 * :Alice in Wonderland

# Channel operations
JOIN #general
PRIVMSG #general :Hello everyone!
TOPIC #general :Welcome to the general channel

# Private messaging
PRIVMSG bob :Hey Bob, how are you?

# Operator commands (if you're a channel operator)
MODE #general +i          # Make channel invite-only
INVITE charlie #general   # Invite Charlie
MODE #general +o bob      # Make Bob an operator
KICK #general troublemaker :Bye
MODE #general +l 50       # Set user limit to 50
MODE #general +k secret123 # Set channel password

# Leave
PART #general :Goodbye!
QUIT :See you later
```

## Technical Details

### Architecture
- **Language**: C++98 standard compliant
- **I/O Model**: Non-blocking I/O with poll() system call
- **Connection**: TCP/IP (IPv4/IPv6 support)
- **Concurrency**: Single-process event loop handling multiple clients
- **Protocol**: IRC Protocol (RFC 1459 / RFC 2812 compliant)

### Design Patterns
- **Orthodox Canonical Form**: All classes follow C++ best practices
- **Event-Driven Architecture**: poll()-based event loop
- **Command Pattern**: Modular command handlers
- **Observer Pattern**: Channel message broadcasting

### Memory Management
- Manual memory management (no smart pointers - C++98)
- Proper cleanup on client disconnect
- Channel auto-deletion when empty
- No memory leaks (verified with valgrind)

### Error Handling
- Comprehensive IRC error codes (ERR_*)
- Graceful degradation on errors
- Proper cleanup on SIGINT/SIGQUIT
- Validation of all user input

## Resources

### IRC Protocol Documentation
- [RFC 1459](https://tools.ietf.org/html/rfc1459) - Original IRC Protocol
- [RFC 2812](https://tools.ietf.org/html/rfc2812) - IRC Client Protocol  
- [Modern IRC Documentation](https://modern.ircdocs.horse/) - Up-to-date IRC reference
- [IRC Numeric Replies](https://www.alien.net.au/irc/irc2numerics.html) - Complete list of IRC codes

### Network Programming
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) - Socket programming bible
- [poll() man page](https://man7.org/linux/man-pages/man2/poll.2.html) - System call documentation
- [TCP/IP Sockets in C](https://cs.baylor.edu/~donahoo/practical/CSockets/) - Practical guide

### Testing Tools
- [irssi](https://irssi.org/) - Terminal IRC client
- [WeeChat](https://weechat.org/) - Modern terminal IRC client
- [HexChat](https://hexchat.github.io/) - GUI IRC client
- [netcat](http://netcat.sourceforge.net/) - TCP/IP swiss army knife

### AI Usage
AI tools were used ethically and responsibly during this project:

**Areas where AI assisted:**
- **Architecture Planning**: Helped design class relationships and project structure
- **IRC Protocol Understanding**: Clarified IRC message formats and numeric reply codes
- **Code Review**: Identified potential bugs and edge cases
- **Documentation**: Assisted in formatting and organizing project documentation
- **Testing Strategies**: Suggested test cases and edge conditions

**Development Process:**
1. All  information were thoroughly reviewed and understood
2. Peer review was conducted to ensure correctness
3. All implementations were tested and debugged by us with the help of AI when needed
4. Unique naming conventions and structures were employed throughout

**Learning Outcomes:**
- Deep understanding of socket programming and non-blocking I/O
- Mastery of the IRC protocol and client-server architecture
- Experience with event-driven programming and poll()
- Improved C++98 skills and memory management
- Better understanding of network protocols and TCP/IP

## Troubleshooting

### Port Already in Use
```bash
# Find process using port
lsof -i :6667
# Kill the process
kill -9 <PID>
```

### Connection Refused
- Check if server is running
- Verify correct port number
- Check firewall settings

### Authentication Failed
- Verify password is correct
- Ensure PASS command is sent first
- Check server logs for errors

### Cannot Join Channel
- Check if channel requires password (MODE +k)
- Verify you're not banned (not implemented)
- Check if channel is full (MODE +l)
- Ensure channel is not invite-only (MODE +i) or you're invited

## Authors

phoang
anmerten
42 School - Luxembourg

## License

This project is part of the 42 School curriculum and follows the school's academic policies.
