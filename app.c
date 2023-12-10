#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WIN32) || defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #include <windows.h>
    #include <winioctl.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>

    #define INVALID_SOCKET_HANDLE (INVALID_SOCKET)
    #define INVALID_SOCKET_RESULT (SOCKET_ERROR)

    typedef SOCKET socket_handle;

    enum {
        SHUTDOWN_RECV = SD_RECEIVE,
        SHUTDOWN_SEND = SD_SEND,
        SHUTDOWN_BOTH = SD_BOTH
    };

    static inline void print_error(const char *message) {
        fprintf(stderr, "%s: %s\n", message, gai_strerrorA(WSAGetLastError()));
    }

    static inline int initialize_sockets(void) {
        WSADATA ws = {0};
        return !(WSAStartup(MAKEWORD(2, 2), &ws));
    }

    static inline void deinitialize_sockets(void) {
        WSACleanup();
    }

    static inline int close_socket(socket_handle socket) {
        return closesocket(socket);
    }
#else
    #include <sys/socket.h>

    #define INVALID_SOCKET_HANDLE (-1)
    #define INVALID_SOCKET_RESULT (-1)

    typedef int socket_handle;

    enum {
        SHUTDOWN_RECV = SHUT_RD,
        SHUTDOWN_SEND = SHUT_WR,
        SHUTDOWN_BOTH = SHUT_RDWR
    };

    static inline void print_error(const char *message) {
        perror(message);
    }

    static inline int initialize_sockets(void) {
        return 1;
    }

    static inline void deinitialize_sockets(void) {}

    static inline int close_socket(socket_handle socket) {
        return close(socket);
    }
#endif

static inline void print_message_with_ipv4(const char *message, const struct sockaddr *addr) {
    struct sockaddr_in sa;
    memcpy(&sa, addr, sizeof(sa));
    char ip[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &sa.sin_addr, ip, sizeof(ip));
    printf("%s%s:%hu\n", message, ip, ntohs(sa.sin_port));
}

int main(void) {
    int result = EXIT_SUCCESS;

    if (!initialize_sockets()) {
        fprintf(stderr, "could not initialize Windows Sockets\n");
        goto close;
    }

    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE,
    };

    struct addrinfo *servinfo = NULL;
    if (getaddrinfo(NULL, "6543", &hints, &servinfo)) {
        print_error("getaddrinfo");
        result = EXIT_FAILURE;
        goto deinit;
    }

    socket_handle sock = INVALID_SOCKET_HANDLE;
    for (struct addrinfo *p = servinfo; p; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock == INVALID_SOCKET_HANDLE) continue;

        if (bind(sock, p->ai_addr, (int)p->ai_addrlen)) {
            print_error("bind");
            close_socket(sock);
            sock = INVALID_SOCKET_HANDLE;
            continue;
        }
        print_message_with_ipv4("listening on ", p->ai_addr);

        break;
    }
    freeaddrinfo(servinfo);

    if (sock == INVALID_SOCKET_HANDLE) {
        fprintf(stderr, "could not create socket\n");
        result = EXIT_FAILURE;
        goto deinit;
    }

    if (listen(sock, 0)) {
        print_error("listen");
        result = EXIT_FAILURE;
        goto close_host;
    }

    struct sockaddr_storage client_addr = {0};
    int client_addr_size = (int)sizeof(client_addr);
    socket_handle client_socket = accept(sock, (struct sockaddr *)&client_addr, &client_addr_size);
    if (client_socket == INVALID_SOCKET_HANDLE) {
        print_error("accept");
        result = EXIT_FAILURE;
        goto close_host;
    }
    print_message_with_ipv4("accepted connection from ", (struct sockaddr *)&client_addr);

    // If an Nginx request isn't read here, we'll have a problem later on when
    // trying to close the client socket.
    //
    // Having any unread socket data results in a TCP RST message being sent to
    // Nginx, which results in a 502 Bad Gateway.
    //
    // Note that we're assuming that everything is read with a single `recv`, in
    // theory that's not necessarily the case.
    char buf[4096];
    if (recv(client_socket, buf, sizeof(buf), 0) == INVALID_SOCKET_RESULT) {
        print_error("recv");
        result = EXIT_FAILURE;
        goto close_all;
    }

    const char response[] = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html\r\n"
                            "\r\n"
                            "<h1>Hello, world!</h1>\n";
    if (send(client_socket, response, sizeof(response) - 1, 0) == INVALID_SOCKET_RESULT) {
        print_error("send");
        result = EXIT_FAILURE;
        goto close_all;
    }

close_all:
    close_socket(client_socket);
close_host:
    close_socket(sock);
deinit:
    deinitialize_sockets();
close:
    return result;
}
