/*
* Copyright (c) 1985, 1989, 1993
*    The Regents of the University of California.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. All advertising materials mentioning features or use of this software
*    must display the following acknowledgement:
* This product includes software developed by the University of
* California, Berkeley and its contributors.
* 4. Neither the name of the University nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

/*
* Portions Copyright (c) 1993 by Digital Equipment Corporation.
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies, and that
* the name of Digital Equipment Corporation not be used in advertising or
* publicity pertaining to distribution of the document or software without
* specific, written prior permission.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
* WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
* CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
* DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
* PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
* ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
* SOFTWARE.
*/

/*
* Portions Copyright (c) 1996 by Internet Software Consortium.
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
* ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
* CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
* DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
* PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
* ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
* SOFTWARE.
*/

#ifndef __pingtel_on_posix__
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_send.c  8.1 (Berkeley) 6/4/93";
static char orig_rcsid[] = "From: Id: res_send.c,v 8.20 1998/04/06 23:27:51 halley Exp $";
static char rcsid[] = "";
#endif /* LIBC_SCCS and not lint */

/*
* Send query to name server and wait for reply.
*/

#ifdef WINCE
#   include <types.h>
#else
#   include <sys/types.h>
#endif
#ifndef WINCE /* no errno.h under WinCE */
#include <errno.h>
#endif 

#include <stdio.h>
#include "resparse/types.h" /* added to pick up NFDBITS and fd_mask --GAT */
#include <time.h>

#include <os/OsMutexC.h>

/* Reordered includes and separated into win/vx --GAT */
#if defined(_WIN32)
#   include <resparse/wnt/sys/param.h>
#   include <winsock2.h>
#   include <resparse/wnt/netinet/in.h>
#   include <resparse/wnt/arpa/nameser.h>
#   include <resparse/wnt/arpa/inet.h>
#   include <resparse/wnt/resolv/resolv.h>
#   include "resparse/wnt/nterrno.h"  /* Additional errors not in errno.h --GAT */
#   include <resparse/wnt/sys/uio.h>
#ifndef WINCE
#   include <io.h>
#endif
#   include "resparse/wnt/res_signal.h"
#elif defined(_VXWORKS)
#   include <netdb.h>
#   include <netinet/in.h>
/* Use local lnameser.h for info missing from VxWorks version --GAT */
/* lnameser.h is a subset of resparse/wnt/arpa/nameser.h                */
#   include <resolv/nameser.h>
#   include <resparse/vxw/arpa/lnameser.h>
#   include <arpa/inet.h>
/* Use local lresolv.h for info missing from VxWorks version --GAT */
/* lresolv.h is a subset of resparse/wnt/resolv/resolv.h               */
#   include <resolv/resolv.h>
#   include <resparse/vxw/resolv/lresolv.h>
#   include <unistd.h>
#   include <net/uio.h>
#   include <ioLib.h>
#   include <sockLib.h>
#   include <signal.h> /* for sigaction --GAT */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resparse/poll.h"

#include "resparse/bzero.h"
#include "resparse/res_config.h"
#include <os/OsDefs.h>
extern struct __res_state _sip_res ;
extern OsMutexC resGlobalLock;

/*defined in OsSocket*/
unsigned long osSocketGetDefaultBindAddress();

/* We defined NOPOLL */

#define NOPOLL

#if defined(_WIN32) /* only needed for win32 --GAT */
int poll(struct pollfd *_pfd, unsigned _nfds, int _timeout)
{
   return 0;
}

int sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
   return 0;
}
#endif


typedef struct res_send_state
{
   int s;              /* socket used for communications */
   int connected;      /* is the socket connected */
   int vc;             /* is the socket a virtual circuit? */
} res_send_state;

#define CAN_RECONNECT 1

#ifndef DEBUG
#   define Dprint(cond, args) /*empty*/
#   define DprintQ(cond, args, query, size) /*empty*/
#   define Aerror(file, string, error, address) /*empty*/
#   define Perror(file, string, error) /*empty*/
#else
#   define Dprint(cond, args) if (cond) {fprintf args;} else {}
#   define DprintQ(cond, args, query, size) if (cond) {\
   fprintf args;\
} else {}
/*#   define DprintQ(cond, args, query, size) if (cond) {\
fprintf args;\
__fp_nquery(query, size, stdout);\
} else {} */
static void Aerror(FILE *file, char *string, int error, struct sockaddr_in address)
{
   int save = errno;
#ifndef WINCE
   if (_sip_res.options & RES_DEBUG) {
      fprintf(file, "res_send: %s ([%s].%u): %s\n",
         string,
         inet_ntoa(address.sin_addr),
         ntohs(address.sin_port),
         strerror(error));
   }
#endif
   errno = save;
}
static void Perror(FILE *file, char *string, int error)
{
   int save = errno;
#ifndef WINCE
   if (_sip_res.options & RES_DEBUG) {
      fprintf(file, "res_send: %s: %s\n",
         string, strerror(error));
   }
#endif
   errno = save;
}
#endif

void init_res_send_state(res_send_state *state)
{
   state->s = -1;
   state->connected = 0;
   state->vc = 0;
}

/*
* This routine is for closing the socket if a virtual circuit is used and
* the program wants to close it.  This provides support for endhostent()
* which expects to close the socket.
*
* This routine is not expected to be user visible.
*/
void res_close(res_send_state *state)
{
   if (state)
   {
      if (state->s >= 0)
      {
#if defined(_WIN32)
         (void) closesocket(state->s);
#else
         (void) close(state->s);
#endif

         state->s = -1;
         state->connected = 0;
         state->vc = 0;
      }
   }   
}

/* int
* res_isourserver_local(ina)
* looks up "ina" in _res.ns_addr_list[]
* returns:
* 0  : not found
* >0 : found
* author:
* paul vixie, 29may94
*/
int res_isourserver_local(const struct sockaddr_in *inp)   /* name conflict: appended _local */
{
   struct sockaddr_in ina;
   int ns, ret;

   ina = *inp;
   ret = 0;

   acquireMutex(resGlobalLock);
   for (ns = 0;  ns < _sip_res.nscount;  ns++)
   {
      const struct sockaddr_in *srv = &_sip_res.nsaddr_list[ns];

      if (srv->sin_family == ina.sin_family &&
         srv->sin_port == ina.sin_port &&
         (srv->sin_addr.s_addr == osSocketGetDefaultBindAddress() ||
         srv->sin_addr.s_addr == ina.sin_addr.s_addr))
      {
            ret++;
            break;
      }
   }

   releaseMutex(resGlobalLock);
   return (ret);
}

/* int
* res_nameinquery_local(name, type, class, buf, eom)
*	look for (name,type,class) in the query section of packet (buf,eom)
* requires:
* buf + HFIXEDSZ <= eom
* returns:
* -1 : format error
* 0  : not found
*>0 : found
* author:
*  paul vixie, 29may94
*/
int res_nameinquery_local(const char *name,
                          int type,
                          int class,
                          const u_char *buf,
                          const u_char *eom) /* name conflict, appended _local */
{
   const u_char *cp = buf + HFIXEDSZ;
   int qdcount = ntohs((u_short)((HEADER*)buf)->qdcount);

   while (qdcount-- > 0)
   {
      char tname[MAXDNAME+1];
      int n, ttype, tclass;

      n = dn_expand(buf, eom, cp, tname, sizeof tname);
      if (n < 0)
         return (-1);
      cp += n;
      if (cp + 2 * INT16SZ > eom)
         return (-1);
      ttype = ns_get16(cp); cp += INT16SZ;
      tclass = ns_get16(cp); cp += INT16SZ;
      if (ttype == type &&
         tclass == class &&
         _stricmp(tname, name) == 0)
         return (1);
   }
   return (0);
}

/* int
* res_queriesmatch_local(buf1, eom1, buf2, eom2)
* is there a 1:1 mapping of (name,type,class)
* in (buf1,eom1) and (buf2,eom2)?
* returns:
* -1 : format error
* 0  : not a 1:1 mapping
* >0 : is a 1:1 mapping
* author:
* paul vixie, 29may94
*/
int res_queriesmatch_local(const u_char *buf1,
                           const u_char *eom1,
                           const u_char *buf2,
                           const u_char *eom2)
{
   const u_char *cp = buf1 + HFIXEDSZ;
   int qdcount = ntohs((u_short)((HEADER*)buf1)->qdcount);

   if (buf1 + HFIXEDSZ > eom1 || buf2 + HFIXEDSZ > eom2)
      return (-1);

   /*
   * Only header section present in replies to
   * dynamic update packets.
   */
   if ( (((HEADER *)buf1)->opcode == ns_o_update) &&
      (((HEADER *)buf2)->opcode == ns_o_update) )
      return (1);

   if (qdcount != ntohs((u_short)((HEADER*)buf2)->qdcount))
      return (0);
   while (qdcount-- > 0)
   {
      char tname[MAXDNAME+1];
      int n, ttype, tclass;

      n = dn_expand(buf1, eom1, cp, tname, sizeof tname);
      if (n < 0)
         return (-1);
      cp += n;
      if (cp + 2 * INT16SZ > eom1)
         return (-1);
      ttype = ns_get16(cp);  cp += INT16SZ;
      tclass = ns_get16(cp); cp += INT16SZ;
      if (!res_nameinquery_local(tname, ttype, tclass, buf2, eom2))
         return (0);
   }
   return (1);
}

int res_send(const u_char *buf,
             int buflen,
             u_char *ans,
             int anssiz)
{
   res_send_state send_state;

#ifndef NOPOLL                  /* libc_r doesn't wrap poll yet() */
   int use_poll = 1;        /* adapt to poll() syscall availability */
   /* 0 = not present, 1 = try it, 2 = exists */
#endif

#if defined(_WIN32)  /* Added for debugging --GAT */
   int socket_error = 0;
   int send_error = 0;
#elif defined(_VXWORKS)
   u_char tempchar;
#endif

   HEADER *hp = (HEADER *) buf;
   HEADER *anhp = (HEADER *) ans;
   int gotsomewhere, connreset, terrno, itry, v_circuit, resplen, ns, n;
   int nstimeout = 0;
   u_int badns; /* XXX NSMAX can't exceed #/bits in this variable */
   int sip_res_retry;
   int sip_res_nscount;
   int sip_res_retrans;
   int sip_res_pfcode;
   int sip_res_options;
   int i = 0;

   struct sockaddr_in nsaddr_list[MAXNS];

   init_res_send_state(&send_state);

   acquireMutex(resGlobalLock);
   sip_res_retry = _sip_res.retry;
   sip_res_nscount = _sip_res.nscount;
   sip_res_retrans = _sip_res.retrans;
   sip_res_pfcode = _sip_res.pfcode;
   sip_res_options = _sip_res.options;

   for (i = 0; (i < MAXNS) && (i < sip_res_nscount); i++)
   {
      nsaddr_list[i] = _sip_res.nsaddr_list[i];
   }
   sip_res_nscount = i;

   if ((sip_res_options & RES_INIT) == 0 && res_init() == -1)
   {
      /* errno should have been set by res_init() in this case. */
      releaseMutex(resGlobalLock);
      return (-1);
   }
   releaseMutex(resGlobalLock);

   if (anssiz < HFIXEDSZ)
   {
      errno = EINVAL;
      return (-1);
   }
   DprintQ((sip_res_options & RES_DEBUG) || (sip_res_pfcode & RES_PRF_QUERY),
      (stdout, ";; res_send()\n"), buf, buflen);
   v_circuit = (sip_res_options & RES_USEVC) || buflen > PACKETSZ;
   gotsomewhere = 0;
   connreset = 0;
   terrno = ETIMEDOUT;
   badns = 0;

   /*
   * Send request, RETRY times, or until successful
   */
   for (itry = 0; itry < sip_res_retry; itry++)
   {
      for (ns = 0; ns < sip_res_nscount; ns++)
      {
         struct sockaddr_in *nsap = &nsaddr_list[ns];
same_ns:
         if (badns & (1 << ns))
         {
            res_close(&send_state);
            goto next_ns;
         }

         Dprint(sip_res_options & RES_DEBUG,
            (stdout, ";; Querying server (# %d) address = %s\n",
            ns + 1, inet_ntoa(nsap->sin_addr)));

         if (v_circuit)
         {
            int truncated;
            struct iovec iov[2];
            u_short len;
            u_char *cp;

            /*
            * Use virtual circuit;
            * at most one attempt per server.
            */
            itry = sip_res_retry;
            truncated = 0;
            if (send_state.s < 0 || !send_state.vc || hp->opcode == ns_o_update)
            {
               if (send_state.s >= 0)
                  res_close(&send_state);

               send_state.s = socket(PF_INET, SOCK_STREAM, 0);
               if (send_state.s < 0)
               {
                  terrno = errno;
                  Perror(stderr, "socket(vc)", errno);
                  return (-1);
               }
               errno = 0;
               if (connect(send_state.s, (struct sockaddr *)nsap,
                  sizeof *nsap) < 0)
               {
                     terrno = errno;
                     Aerror(stderr, "connect/vc",
                        errno, *nsap);
                     badns |= (1 << ns);
                     res_close(&send_state);
                     goto next_ns;
               }
               send_state.vc = 1;
            }
            /*
            * Send length & message
            */
            putshort((u_short)buflen, (u_char*)&len);
            iov[0].iov_base = (caddr_t)&len;
            iov[0].iov_len = INT16SZ;
            iov[1].iov_base = (caddr_t)buf;
            iov[1].iov_len = buflen;
            if (writev(send_state.s, iov, 2) != (INT16SZ + buflen))
            {
               terrno = errno;
               Perror(stderr, "write failed", errno);
               badns |= (1 << ns);
               res_close(&send_state);
               goto next_ns;
            }
            /*
            * Receive length & response
            */
read_len:
            cp = ans;
            len = INT16SZ;
            while ((n = recv(send_state.s, (char *)cp, (int)len, 0)) > 0)
            {
               cp += n;
               if ((len -= n) <= 0)
                  break;
            }
            if (n <= 0)
            {
               terrno = errno;
               Perror(stderr, "read failed", errno);
               res_close(&send_state);
               /*
               * A long running process might get its TCP
               * connection reset if the remote server was
               * restarted.  Requery the server instead of
               * trying a new one.  When there is only one
               * server, this means that a query might work
               * instead of failing.  We only allow one reset
               * per query to prevent looping.
               */
               if (terrno == ECONNRESET && !connreset)
               {
                  connreset = 1;
                  res_close(&send_state);
                  goto same_ns;
               }
               res_close(&send_state);
               goto next_ns;
            }
            resplen = ns_get16(ans);
            if (resplen > anssiz)
            {
               Dprint(sip_res_options & RES_DEBUG,
                  (stdout, ";; response truncated\n")
                  );
               truncated = 1;
               len = anssiz;
            } else
               len = resplen;
            if (len < HFIXEDSZ)
            {
               /*
               * Undersized message.
               */
               Dprint(sip_res_options & RES_DEBUG,
                  (stdout, ";; undersized: %d\n", len));
               terrno = EMSGSIZE;
               badns |= (1 << ns);
               res_close(&send_state);
               goto next_ns;
            }
            cp = ans;
            while (len != 0 &&
               (n = recv(send_state.s, (char *)cp, (int)len, 0)) > 0)
            {
                  cp += n;
                  len -= n;
            }
            if (n <= 0)
            {
               terrno = errno;
               Perror(stderr, "read(vc)", errno);
               res_close(&send_state);
               goto next_ns;
            }
            if (truncated)
            {
               /*
               * Flush rest of answer
               * so connection stays in synch.
               */
               anhp->tc = 1;
               len = resplen - anssiz;
               while (len != 0)
               {
                  char junk[PACKETSZ];

                  n = (len > sizeof(junk)
                     ? sizeof(junk)
                     : len);
                  if ((n = recv(send_state.s, junk, n, 0)) > 0)
                     len -= n;
                  else
                     break;
               }
            }
            /*
            * The calling applicating has bailed out of
            * a previous call and failed to arrange to have
            * the circuit closed or the server has got
            * itself confused. Anyway drop the packet and
            * wait for the correct one.
            */
            if (hp->id != anhp->id)
            {
               DprintQ((sip_res_options & RES_DEBUG) ||
                  (sip_res_pfcode & RES_PRF_REPLY),
                  (stdout, ";; old answer (unexpected):\n"),
                  ans, (resplen>anssiz)?anssiz:resplen);
               goto read_len;
            }
         }
         else
         {
            /*
            * Use datagrams.
            */
#ifndef NOPOLL
            struct pollfd pfd;
            int msec;
#endif
            struct timeval timeout;
            fd_set dsmask, *dsmaskp;
            int dsmasklen;
            struct sockaddr_in from;
            int fromlen;

            // if no socket exists, create one
            if ((send_state.s < 0) || send_state.vc)
            {
               if (send_state.vc)
                  res_close(&send_state);

               send_state.s = socket(PF_INET, SOCK_DGRAM, 0);
#if defined(_WIN32)  /* Added for debugging --GAT */
               if (send_state.s == INVALID_SOCKET)
               {
                  socket_error = WSAGetLastError();
                  Dprint(sip_res_options & RES_DEBUG,
                     (stdout, "socket() call failed with error: %d\n",
                     socket_error));
                  return (-1);
               }
#endif
               if (send_state.s < 0)
               {
#ifndef CAN_RECONNECT
bad_dg_sock:
#endif
                  terrno = errno;
                  Perror(stderr, "socket(dg)", errno);
                  return (-1);
               }
               send_state.connected = 0;
            }

#ifndef CANNOT_CONNECT_DGRAM
            /*
            * On a 4.3BSD+ machine (client and server,
            * actually), sending to a nameserver datagram
            * port with no nameserver will cause an
            * ICMP port unreachable message to be returned.
            * If our datagram socket is "connected" to the
            * server, we get an ECONNREFUSED error on the next
            * socket operation, and select returns if the
            * error message is received.  We can thus detect
            * the absence of a nameserver without timing out.
            * If we have sent queries to at least two servers,
            * however, we don't want to remain connected,
            * as we wish to receive answers from the first
            * server to respond.
            */
            if (sip_res_nscount == 1 || (itry == 0 && ns == 0))
            {
               /*
               * Connect only if we are sure we won't
               * receive a response from another server.
               */
               if (!send_state.connected)
               {
                  if (connect(send_state.s, (struct sockaddr *)nsap, sizeof *nsap) < 0)
                  {
                        Aerror(stderr,
                           "connect(dg)",
                           errno, *nsap);
                        badns |= (1 << ns);
                        res_close(&send_state);
                        goto next_ns;
                  }
                  send_state.connected = 1;
               }
               if ( (send(send_state.s, (char*)buf, buflen, 0) ) != buflen)
               {
#if defined(_WIN32)  /* Added for debugging --GAT */
                  send_error = WSAGetLastError();
                  Dprint(sip_res_options & RES_DEBUG,
                     (stdout, "send() call failed with error: %d\n",
                     send_error));
#endif
                  Perror(stderr, "send", errno);
                  badns |= (1 << ns);
                  res_close(&send_state);
                  goto next_ns;
               }
            }
            else
            {
               /*
               * Disconnect if we want to listen
               * for responses from more than one server.
               */
               if (send_state.connected)
               {
#ifdef CAN_RECONNECT
                  struct sockaddr_in no_addr;
                  no_addr.sin_family = AF_INET;
                  no_addr.sin_addr.s_addr = osSocketGetDefaultBindAddress();
                  /*					no_addr.sin_addr.s_addr = INADDR_ANY;*/
                  no_addr.sin_port = 0;
                  connect(send_state.s, (struct sockaddr *)&no_addr, sizeof no_addr);
#else
                  int s1 = socket(PF_INET, SOCK_DGRAM,0);
                  if (s1 < 0)
                     goto bad_dg_sock;
                  (void) dup2(s1, s);
                  (void) close(s1);
                  Dprint(sip_res_options & RES_DEBUG,
                     (stdout, ";; new DG socket\n"))
#endif /* CAN_RECONNECT */
                     send_state.connected = 0;
                  errno = 0;
               }
#endif /* !CANNOT_CONNECT_DGRAM */
               if (sendto(send_state.s, (char*)buf, buflen, 0,
                  (struct sockaddr *)nsap,
                  sizeof *nsap)
                  != buflen) {
                     Aerror(stderr, "sendto", errno, *nsap);
                     badns |= (1 << ns);
                     res_close(&send_state);
                     goto next_ns;
               }
#ifndef CANNOT_CONNECT_DGRAM
            }
#endif /* !CANNOT_CONNECT_DGRAM */

            /*
            * Wait for reply
            */
#ifndef NOPOLL
othersyscall:
            if (use_poll) {
               msec = (sip_res_retrans << itry) * 1000;
               if (itry > 0)
                  msec /= sip_res_nscount;
               if (msec <= 0)
                  msec = 1000;
            } else {
#endif
               timeout.tv_sec = (sip_res_retrans << itry);
               if (itry > 0)
                  timeout.tv_sec /= sip_res_nscount;
               if ((long) timeout.tv_sec <= 0)
                  timeout.tv_sec = 1;
               timeout.tv_usec = 0;
#ifndef NOPOLL
            }
#endif
wait:
            if (send_state.s < 0) {
               Perror(stderr, "s out-of-bounds", EMFILE);
               res_close(&send_state);
               goto next_ns;
            }
#ifndef NOPOLL
            if (use_poll) {
               struct sigaction sa, osa;
               int sigsys_installed = 0;

               pfd.fd = send_state.s;
               pfd.events = POLLIN;
               if (use_poll == 1) {
                  bzero_local(&sa, sizeof(sa));
                  sa.sa_handler = SIG_IGN;
                  if (sigaction(SIGSYS, &sa, &osa) >= 0)
                     sigsys_installed = 1;
               }
               n = poll(&pfd, 1, msec);
               if (sigsys_installed == 1) {
                  int oerrno = errno;
                  sigaction(SIGSYS, &osa, NULL);
                  errno = oerrno;
               }
               /* XXX why does nosys() return EINVAL? */
               if (n < 0 && (errno == ENOSYS ||
                  errno == EINVAL)) {
                     use_poll = 0;
                     goto othersyscall;
               } else if (use_poll == 1)
                  use_poll = 2;
               if (n < 0) {
                  if (errno == EINTR)
                     goto wait;
                  Perror(stderr, "poll", errno);
                  res_close(&send_state);
                  goto next_ns;
               }
            } else {
#endif
               dsmasklen = howmany(send_state.s + 1, NFDBITS) *
                  sizeof(fd_mask);
               if (dsmasklen > (int) (sizeof(fd_set))) {
                  dsmaskp = (fd_set *)malloc(dsmasklen);
                  if (dsmaskp == NULL) {
                     res_close(&send_state);
                     goto next_ns;
                  }
               } else
                  dsmaskp = &dsmask;
               /* only zero what we need */
               bzero_local((char *)dsmaskp, dsmasklen);
               FD_SET((u_int)send_state.s, dsmaskp);
               n = select(send_state.s + 1, dsmaskp, (fd_set *)NULL,
                  (fd_set *)NULL, &timeout);
               if (dsmaskp != &dsmask)
                  free(dsmaskp);
               if (n < 0) {
                  if (errno == EINTR)
                     goto wait;
                  Perror(stderr, "select", errno);
                  res_close(&send_state);
                  goto next_ns;
               }
#ifndef NOPOLL
            }
#endif

            if (n == 0) {
               /*
               * timeout
               */
               nstimeout = 1;
               Dprint(sip_res_options & RES_DEBUG,
                  (stdout, ";; timeout\n"));
               gotsomewhere = 1;
               res_close(&send_state);
               goto next_ns;
            }
            errno = 0;
            fromlen = sizeof(struct sockaddr_in);
            resplen = recvfrom(send_state.s, (char*)ans, anssiz, 0,
               (struct sockaddr *)&from, &fromlen);
            if (resplen <= 0) {
               Perror(stderr, "recvfrom", errno);
               res_close(&send_state);
               goto next_ns;
            }
#if defined(_VXWORKS)
            /* OK, let's "byte-swap" the len/family fields in VxWorks to get the data right.
            * For some reason the data is coming in reversed from recvfrom.  Note that
            * this isn't a problem on a win32 machine. --GAT
            */
            tempchar = from.sin_len;
            from.sin_len = from.sin_family;
            from.sin_family = tempchar;
#endif
            gotsomewhere = 1;
            if (resplen < HFIXEDSZ) {
               /*
               * Undersized message.
               */
               Dprint(sip_res_options & RES_DEBUG,
                  (stdout, ";; undersized: %d\n",
                  resplen));
               terrno = EMSGSIZE;
               badns |= (1 << ns);
               res_close(&send_state);
               goto next_ns;
            }
            if (hp->id != anhp->id) {
               /*
               * response from old query, ignore it.
               * XXX - potential security hazard could
               *	 be detected here.
               */
               DprintQ((sip_res_options & RES_DEBUG) ||
                  (sip_res_pfcode & RES_PRF_REPLY),
                  (stdout, ";; old answer:\n"),
                  ans, (resplen>anssiz)?anssiz:resplen);
               goto wait;
            }
#ifdef CHECK_SRVR_ADDR
            if (!(sip_res_options & RES_INSECURE1) &&
               !res_isourserver_local(&from)) {
                  /*
                  * response from wrong server? ignore it.
                  * XXX - potential security hazard could
                  *	 be detected here.
                  */
                  DprintQ((sip_res_options & RES_DEBUG) ||
                     (sip_res_pfcode & RES_PRF_REPLY),
                     (stdout, ";; not our server:\n"),
                     ans, (resplen>anssiz)?anssiz:resplen);
                  goto wait;
            }
#endif
            if (!(sip_res_options & RES_INSECURE2) &&
               !res_queriesmatch_local(buf, buf + buflen,  /* wdn - _local */
               ans, ans + anssiz)) {
                  /*
                  * response contains wrong query? ignore it.
                  * XXX - potential security hazard could
                  *	 be detected here.
                  */
                  DprintQ((sip_res_options & RES_DEBUG) ||
                     (sip_res_pfcode & RES_PRF_REPLY),
                     (stdout, ";; wrong query name:\n"),
                     ans, (resplen>anssiz)?anssiz:resplen);
                  goto wait;
            }
            if (anhp->rcode == SERVFAIL ||
               anhp->rcode == NOTIMP ||
               anhp->rcode == REFUSED) {
                  DprintQ(sip_res_options & RES_DEBUG,
                     (stdout, "server rejected query:\n"),
                     ans, (resplen>anssiz)?anssiz:resplen);
                  badns |= (1 << ns);
                  res_close(&send_state);
                  /* don't retry if called from dig */
                  if (!sip_res_pfcode)
                     goto next_ns;
            }
            if (!(sip_res_options & RES_IGNTC) && anhp->tc) {
               /*
               * get rest of answer;
               * use TCP with same server.
               */
               Dprint(sip_res_options & RES_DEBUG,
                  (stdout, ";; truncated answer\n"));
               v_circuit = 1;
               res_close(&send_state);
               goto same_ns;
            }
         } /*if vc/dg*/
         Dprint((sip_res_options & RES_DEBUG) ||
            ((sip_res_pfcode & RES_PRF_REPLY) &&
            (sip_res_pfcode & RES_PRF_HEAD1)),
            (stdout, ";; got answer:\n"));
         DprintQ((sip_res_options & RES_DEBUG) ||
            (sip_res_pfcode & RES_PRF_REPLY),
            (stdout, ""),
            ans, (resplen>anssiz)?anssiz:resplen);
         /*
         * If using virtual circuits, we assume that the first server
         * is preferred over the rest (i.e. it is on the local
         * machine) and only keep that one open.
         * If we have temporarily opened a virtual circuit,
         * or if we haven't been asked to keep a socket open,
         * close the socket.
         */
         if ((v_circuit && (!(sip_res_options & RES_USEVC) || ns != 0)) ||
            !(sip_res_options & RES_STAYOPEN)) {
               res_close(&send_state);
         }
         if (resplen > 0 && nstimeout && ns > 0)
         {
            int k = 0;
            acquireMutex(resGlobalLock);
            // if we got a valid response, and there was a timeout, swap items in nsaddr_list
            // to prevent timeout in the future

            // this is a bit tricky, we have to find the address in real array, not in our copy
            // the position might have already changed from another thread
            for (k = 0; k < _sip_res.nscount; k++)
            {
               if (nsap->sin_addr.S_un.S_addr == _sip_res.nsaddr_list[k].sin_addr.S_un.S_addr)
               {
                  // we found the correct index, now swap
                  struct sockaddr_in tmp = _sip_res.nsaddr_list[k];
                  _sip_res.nsaddr_list[k] = _sip_res.nsaddr_list[0];
                  _sip_res.nsaddr_list[0] = tmp;

                  break;
               }
            }
            // now working DNS address will be 1st
            releaseMutex(resGlobalLock);
         }
         return (resplen);
next_ns: ;
      } /*foreach ns*/
   } /*foreach retry*/
   res_close(&send_state);
   if (!v_circuit) {
      if (!gotsomewhere)
         errno = ECONNREFUSED;  /* no nameservers found */
      else
         errno = ETIMEDOUT;	/* no answer obtained */
   } else
      errno = terrno;
   return (-1);
}
#endif /* __pingtel_on_posix__ */
