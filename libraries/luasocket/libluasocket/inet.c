/*=========================================================================*\
* Internet domain functions
* LuaSocket toolkit
\*=========================================================================*/
#include "luasocket.h"
#include "inet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*=========================================================================*\
* Internal function prototypes.
\*=========================================================================*/
static int inet_global_toip(lua_State *L);
static int inet_global_getaddrinfo(lua_State *L);
static int inet_global_tohostname(lua_State *L);
static int inet_global_getnameinfo(lua_State *L);
static void inet_pushresolved(lua_State *L, struct hostent *hp);
static int inet_global_gethostname(lua_State *L);

/* DNS functions */
static luaL_Reg func[] = {
    { "toip", inet_global_toip},
    { "getaddrinfo", inet_global_getaddrinfo},
    { "tohostname", inet_global_tohostname},
    { "getnameinfo", inet_global_getnameinfo},
    { "gethostname", inet_global_gethostname},
    { NULL, NULL}
};

/*-------------------------------------------------------------------------*\
* Initializes module
\*-------------------------------------------------------------------------*/
int inet_open(lua_State *L)
{
    lua_pushstring(L, "dns");
    lua_newtable(L);
    luaL_setfuncs(L, func, 0);
    lua_settable(L, -3);
    return 0;
}

/*=========================================================================*\
* Global Lua functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Returns all information provided by the resolver given a host name
* or ip address
\*-------------------------------------------------------------------------*/
static int inet_gethost(const char *address, struct hostent **hp) {
    struct in_addr addr;
    if (inet_aton(address, &addr))
        return socket_gethostbyaddr((char *) &addr, sizeof(addr), hp);
    else
        return socket_gethostbyname(address, hp);
}

/*-------------------------------------------------------------------------*\
* Returns all information provided by the resolver given a host name
* or ip address
\*-------------------------------------------------------------------------*/
static int inet_global_tohostname(lua_State *L) {
    const char *address = luaL_checkstring(L, 1);
    struct hostent *hp = NULL;
    int err = inet_gethost(address, &hp);
    if (err != IO_DONE) {
        lua_pushnil(L);
        lua_pushstring(L, socket_hoststrerror(err));
        return 2;
    }
    lua_pushstring(L, hp->h_name);
    inet_pushresolved(L, hp);
    return 2;
}

static int inet_global_getnameinfo(lua_State *L) {
    char hbuf[NI_MAXHOST];
    char sbuf[NI_MAXSERV];
    int i, ret;
    struct addrinfo hints;
    struct addrinfo *resolved, *iter;
    const char *host = luaL_optstring(L, 1, NULL);
    const char *serv = luaL_optstring(L, 2, NULL);

    if (!(host || serv))
        luaL_error(L, "host and serv cannot be both nil");

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    ret = getaddrinfo(host, serv, &hints, &resolved);
    if (ret != 0) {
        lua_pushnil(L);
        lua_pushstring(L, socket_gaistrerror(ret));
        return 2;
    }

    lua_newtable(L);
    for (i = 1, iter = resolved; iter; i++, iter = iter->ai_next) {
        getnameinfo(iter->ai_addr, (socklen_t) iter->ai_addrlen,
            hbuf, host? (socklen_t) sizeof(hbuf): 0,
            sbuf, serv? (socklen_t) sizeof(sbuf): 0, 0);
        if (host) {
            lua_pushnumber(L, i);
            lua_pushstring(L, hbuf);
            lua_settable(L, -3);
        }
    }
    freeaddrinfo(resolved);

    if (serv) {
        lua_pushstring(L, sbuf);
        return 2;
    } else {
        return 1;
    }
}

/*-------------------------------------------------------------------------*\
* Returns all information provided by the resolver given a host name
* or ip address
\*-------------------------------------------------------------------------*/
static int inet_global_toip(lua_State *L)
{
    const char *address = luaL_checkstring(L, 1);
    struct hostent *hp = NULL;
    int err = inet_gethost(address, &hp);
    if (err != IO_DONE) {
        lua_pushnil(L);
        lua_pushstring(L, socket_hoststrerror(err));
        return 2;
    }
    lua_pushstring(L, inet_ntoa(*((struct in_addr *) hp->h_addr)));
    inet_pushresolved(L, hp);
    return 2;
}

int inet_optfamily(lua_State* L, int narg, const char* def)
{
    static const char* optname[] = { "unspec", "inet", "inet6", NULL };
    static int optvalue[] = { AF_UNSPEC, AF_INET, AF_INET6, 0 };

    return optvalue[luaL_checkoption(L, narg, def, optname)];
}

int inet_optsocktype(lua_State* L, int narg, const char* def)
{
    static const char* optname[] = { "stream", "dgram", NULL };
    static int optvalue[] = { SOCK_STREAM, SOCK_DGRAM, 0 };

    return optvalue[luaL_checkoption(L, narg, def, optname)];
}

static int inet_global_getaddrinfo(lua_State *L)
{
    const char *hostname = luaL_checkstring(L, 1);
    struct addrinfo *iterator = NULL, *resolved = NULL;
    struct addrinfo hints;
    int i = 1, ret = 0;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    ret = getaddrinfo(hostname, NULL, &hints, &resolved);
    if (ret != 0) {
        lua_pushnil(L);
        lua_pushstring(L, socket_gaistrerror(ret));
        return 2;
    }
    lua_newtable(L);
    for (iterator = resolved; iterator; iterator = iterator->ai_next) {
        char hbuf[NI_MAXHOST];
        ret = getnameinfo(iterator->ai_addr, (socklen_t) iterator->ai_addrlen,
            hbuf, (socklen_t) sizeof(hbuf), NULL, 0, NI_NUMERICHOST);
        if (ret){
          freeaddrinfo(resolved);
          lua_pushnil(L);
          lua_pushstring(L, socket_gaistrerror(ret));
          return 2;
        }
        lua_pushnumber(L, i);
        lua_newtable(L);
        switch (iterator->ai_family) {
            case AF_INET:
                lua_pushliteral(L, "family");
                lua_pushliteral(L, "inet");
                lua_settable(L, -3);
                break;
            case AF_INET6:
                lua_pushliteral(L, "family");
                lua_pushliteral(L, "inet6");
                lua_settable(L, -3);
                break;
            case AF_UNSPEC:
                lua_pushliteral(L, "family");
                lua_pushliteral(L, "unspec");
                lua_settable(L, -3);
                break;
            default:
                lua_pushliteral(L, "family");
                lua_pushliteral(L, "unknown");
                lua_settable(L, -3);
                break;
        }
        lua_pushliteral(L, "addr");
        lua_pushstring(L, hbuf);
        lua_settable(L, -3);
        lua_settable(L, -3);
        i++;
    }
    freeaddrinfo(resolved);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Gets the host name
\*-------------------------------------------------------------------------*/
static int inet_global_gethostname(lua_State *L)
{
    char name[257];
    name[256] = '\0';
    if (gethostname(name, 256) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, socket_strerror(errno));
        return 2;
    } else {
        lua_pushstring(L, name);
        return 1;
    }
}

/*=========================================================================*\
* Lua methods
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Retrieves socket peer name
\*-------------------------------------------------------------------------*/
int inet_meth_getpeername(lua_State *L, p_socket ps, int family)
{
    int err;
    struct sockaddr_storage peer;
    socklen_t peer_len = sizeof(peer);
    char name[INET6_ADDRSTRLEN];
    char port[6]; /* 65535 = 5 bytes + 0 to terminate it */
    if (getpeername(*ps, (SA *) &peer, &peer_len) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, socket_strerror(errno));
        return 2;
    }
	err = getnameinfo((struct sockaddr *) &peer, peer_len,
        name, INET6_ADDRSTRLEN,
        port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
    if (err) {
        lua_pushnil(L);
        lua_pushstring(L, LUA_GAI_STRERROR(err));
        return 2;
    }
    lua_pushstring(L, name);
    lua_pushinteger(L, (int) strtol(port, (char **) NULL, 10));
    switch (family) {
        case AF_INET: lua_pushliteral(L, "inet"); break;
        case AF_INET6: lua_pushliteral(L, "inet6"); break;
        case AF_UNSPEC: lua_pushliteral(L, "unspec"); break;
        default: lua_pushliteral(L, "unknown"); break;
    }
    return 3;
}

/*-------------------------------------------------------------------------*\
* Retrieves socket local name
\*-------------------------------------------------------------------------*/
int inet_meth_getsockname(lua_State *L, p_socket ps, int family)
{
    int err;
    struct sockaddr_storage peer;
    socklen_t peer_len = sizeof(peer);
    char name[INET6_ADDRSTRLEN];
    char port[6]; /* 65535 = 5 bytes + 0 to terminate it */
    if (getsockname(*ps, (SA *) &peer, &peer_len) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, socket_strerror(errno));
        return 2;
    }
	err=getnameinfo((struct sockaddr *)&peer, peer_len,
		name, INET6_ADDRSTRLEN, port, 6, NI_NUMERICHOST | NI_NUMERICSERV);
    if (err) {
        lua_pushnil(L);
        lua_pushstring(L, LUA_GAI_STRERROR(err));
        return 2;
    }
    lua_pushstring(L, name);
    lua_pushstring(L, port);
    switch (family) {
        case AF_INET: lua_pushliteral(L, "inet"); break;
        case AF_INET6: lua_pushliteral(L, "inet6"); break;
        case AF_UNSPEC: lua_pushliteral(L, "unspec"); break;
        default: lua_pushliteral(L, "unknown"); break;
    }
    return 3;
}

/*=========================================================================*\
* Internal functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Passes all resolver information to Lua as a table
\*-------------------------------------------------------------------------*/
static void inet_pushresolved(lua_State *L, struct hostent *hp)
{
    char **alias;
    struct in_addr **addr;
    int i, resolved;
    lua_newtable(L); resolved = lua_gettop(L);
    lua_pushstring(L, "name");
    lua_pushstring(L, hp->h_name);
    lua_settable(L, resolved);
    lua_pushstring(L, "ip");
    lua_pushstring(L, "alias");
    i = 1;
    alias = hp->h_aliases;
    lua_newtable(L);
    if (alias) {
        while (*alias) {
            lua_pushnumber(L, i);
            lua_pushstring(L, *alias);
            lua_settable(L, -3);
            i++; alias++;
        }
    }
    lua_settable(L, resolved);
    i = 1;
    lua_newtable(L);
    addr = (struct in_addr **) hp->h_addr_list;
    if (addr) {
        while (*addr) {
            lua_pushnumber(L, i);
            lua_pushstring(L, inet_ntoa(**addr));
            lua_settable(L, -3);
            i++; addr++;
        }
    }
    lua_settable(L, resolved);
}

/*-------------------------------------------------------------------------*\
* Tries to create a new inet socket
\*-------------------------------------------------------------------------*/
const char *inet_trycreate(p_socket ps, int family, int type, int protocol) {
    const char *err = socket_strerror(socket_create(ps, family, type, protocol));
    if (err == NULL && family == AF_INET6) {
        int yes = 1;
        setsockopt(*ps, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&yes, sizeof(yes));
    }
    return err;
}

/*-------------------------------------------------------------------------*\
* "Disconnects" a DGRAM socket
\*-------------------------------------------------------------------------*/
const char *inet_trydisconnect(p_socket ps, int family, p_timeout tm)
{
    switch (family) {
        case AF_INET: {
            struct sockaddr_in sin;
            memset((char *) &sin, 0, sizeof(sin));
            sin.sin_family = AF_UNSPEC;
            sin.sin_addr.s_addr = INADDR_ANY;
            return socket_strerror(socket_connect(ps, (SA *) &sin,
                sizeof(sin), tm));
        }
        case AF_INET6: {
            struct sockaddr_in6 sin6;
            struct in6_addr addrany = INADDR_ANY;
            memset((char *) &sin6, 0, sizeof(sin6));
            sin6.sin6_family = AF_UNSPEC;
            sin6.sin6_addr = addrany;
            return socket_strerror(socket_connect(ps, (SA *) &sin6,
                sizeof(sin6), tm));
        }
    }
    return NULL;
}

/*-------------------------------------------------------------------------*\
* Tries to connect to remote address (address, port)
\*-------------------------------------------------------------------------*/
const char *inet_tryconnect(p_socket ps, int *family, const char *address,
        const char *serv, p_timeout tm, struct addrinfo *connecthints)
{
    struct addrinfo *iterator = NULL, *resolved = NULL;
    const char *err = NULL;
    int current_family = *family;
    /* try resolving */
    err = socket_gaistrerror(getaddrinfo(address, serv,
                connecthints, &resolved));
    if (err != NULL) {
        if (resolved) freeaddrinfo(resolved);
        return err;
    }
    for (iterator = resolved; iterator; iterator = iterator->ai_next) {
        timeout_markstart(tm);
        /* create new socket if necessary. if there was no
         * bind, we need to create one for every new family
         * that shows up while iterating. if there was a
         * bind, all families will be the same and we will
         * not enter this branch. */
        if (current_family != iterator->ai_family || *ps == SOCKET_INVALID) {
            socket_destroy(ps);
            err = inet_trycreate(ps, iterator->ai_family,
                iterator->ai_socktype, iterator->ai_protocol);
            if (err) continue;
            current_family = iterator->ai_family;
            /* set non-blocking before connect */
            socket_setnonblocking(ps);
        }
        /* try connecting to remote address */
        err = socket_strerror(socket_connect(ps, (SA *) iterator->ai_addr,
            (socklen_t) iterator->ai_addrlen, tm));
        /* if success or timeout is zero, break out of loop */
        if (err == NULL || timeout_iszero(tm)) {
            *family = current_family;
            break;
        }
    }
    freeaddrinfo(resolved);
    /* here, if err is set, we failed */
    return err;
}

/*-------------------------------------------------------------------------*\
* Tries to accept a socket
\*-------------------------------------------------------------------------*/
const char *inet_tryaccept(p_socket server, int family, p_socket client,
    p_timeout tm) {
	socklen_t len;
	t_sockaddr_storage addr;
    switch (family) {
        case AF_INET6: len = sizeof(struct sockaddr_in6); break;
        case AF_INET: len = sizeof(struct sockaddr_in); break;
        default: len = sizeof(addr); break;
    }
	return socket_strerror(socket_accept(server, client, (SA *) &addr,
        &len, tm));
}

/*-------------------------------------------------------------------------*\
* Tries to bind socket to (address, port)
\*-------------------------------------------------------------------------*/
const char *inet_trybind(p_socket ps, int *family, const char *address,
    const char *serv, struct addrinfo *bindhints) {
    struct addrinfo *iterator = NULL, *resolved = NULL;
    const char *err = NULL;
    int current_family = *family;
    /* translate luasocket special values to C */
    if (strcmp(address, "*") == 0) address = NULL;
    if (!serv) serv = "0";
    /* try resolving */
    err = socket_gaistrerror(getaddrinfo(address, serv, bindhints, &resolved));
    if (err) {
        if (resolved) freeaddrinfo(resolved);
        return err;
    }
    /* iterate over resolved addresses until one is good */
    for (iterator = resolved; iterator; iterator = iterator->ai_next) {
        if (current_family != iterator->ai_family || *ps == SOCKET_INVALID) {
            socket_destroy(ps);
            err = inet_trycreate(ps, iterator->ai_family,
                        iterator->ai_socktype, iterator->ai_protocol);
            if (err) continue;
            current_family = iterator->ai_family;
        }
        /* try binding to local address */
        err = socket_strerror(socket_bind(ps, (SA *) iterator->ai_addr,
            (socklen_t) iterator->ai_addrlen));
        /* keep trying unless bind succeeded */
        if (err == NULL) {
            *family = current_family;
            /* set to non-blocking after bind */
            socket_setnonblocking(ps);
            break;
        }
    }
    /* cleanup and return error */
    freeaddrinfo(resolved);
    /* here, if err is set, we failed */
    return err;
}

/*-------------------------------------------------------------------------*\
* Some systems do not provide these so that we provide our own.
\*-------------------------------------------------------------------------*/
#ifdef LUASOCKET_INET_ATON
int inet_aton(const char *cp, struct in_addr *inp)
{
    unsigned int a = 0, b = 0, c = 0, d = 0;
    int n = 0, r;
    unsigned long int addr = 0;
    r = sscanf(cp, "%u.%u.%u.%u%n", &a, &b, &c, &d, &n);
    if (r == 0 || n == 0) return 0;
    cp += n;
    if (*cp) return 0;
    if (a > 255 || b > 255 || c > 255 || d > 255) return 0;
    if (inp) {
        addr += a; addr <<= 8;
        addr += b; addr <<= 8;
        addr += c; addr <<= 8;
        addr += d;
        inp->s_addr = htonl(addr);
    }
    return 1;
}
#endif

#ifdef LUASOCKET_INET_PTON
int inet_pton(int af, const char *src, void *dst)
{
    struct addrinfo hints, *res;
    int ret = 1;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = af;
    hints.ai_flags = AI_NUMERICHOST;
    if (getaddrinfo(src, NULL, &hints, &res) != 0) return -1;
    if (af == AF_INET) {
        struct sockaddr_in *in = (struct sockaddr_in *) res->ai_addr;
        memcpy(dst, &in->sin_addr, sizeof(in->sin_addr));
    } else if (af == AF_INET6) {
        struct sockaddr_in6 *in = (struct sockaddr_in6 *) res->ai_addr;
        memcpy(dst, &in->sin6_addr, sizeof(in->sin6_addr));
    } else {
        ret = -1;
    }
    freeaddrinfo(res);
    return ret;
}

#endif
