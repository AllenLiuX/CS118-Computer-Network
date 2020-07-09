# CS118-Computer-Network

Proj scores:

Proj1: 100

Proj2: 115

Although the code of my projects is open source, please use my code with reference and credit. Thank you.



# CS118 Note

## Lecture 6

#### TCP

- Best possible functions

#### UDP

- Simpliest, less functions

## Chapter 3

### 3.3 Connectionless Transfer UDP

### 3.4 Principles of Reliable Data Transfer

标准：没有corrupt，lost，并且in-order

假定不会乱序

FSM: Finite State Machine

- RDT 1.0
  - 假定无error
- RDT 2.0 
  - 假定有packet error无ack error
  - ack+nak
  - ARQ：Autonatic Repeat reQuest protocols
- RDT 2.1  靠sender来分辨error
  - sequence number + ACK&NAK
  - 假定有ack error -> resend with prev seq
  - packet error -> NAK, resend with prev seq
  - 无ack corrupt & 是ack -> next packet
- RDT 2.2
  - Assume无lost
  - duplicate ack = nak -> resend
  - 到下一个packet必须ACK with correct seq
- RDT 3.0 Alternating-bit protocol, Stop and Wait protocol
  - Assume有lost
  - timer, bigger than RTT
  - timer太短，duplicate data packet, 收到第二个ack do nothing

#### Pipelining

- $d_{trans} = L/R$
- $U_{sender}=(L/R)/(RTT+L/R)$
- $U_{sender}=(N*L/R)/(RTT+L/R)$

##### Go-Back-N protocol (Sliding-window protocol)

- Why need N for window size? flow control & congestion control
- events-based programming
  - Timeout: resend all packets without ACK
  - call from above: check window size and then send
  - ACK: 之前所有packets都ack (cumulative acknowwledgement), window slides forward
- discard out-of-order packets -> simplicity of receiver buffering
- Receiver: only need to record expectedseqnum
- 只有base packet有timer

##### selective Repeat(SR)

- receiver individually acknowledge packet
- Sender
  - Above call: 超过window size则buffer or return
  - ack如果等于base，move forward
  - 每个seq都需要有timer和ack
- Receiver
  - receiver收到seq等于base，deliver up & move forward，否则buffer
  - 如果在【base-N, base-1】的seq，则也发送对应的ack。因为之前有ack lost的时候sender会resend，需要再来次ack来让sender move forward
  - 否则ignore
- lack of synchtonization: sender and receiver's window size -> seq size ≥ window size. 否则分不清是new packet还是retransmission

### 3.5 Connection-oriented transport: TCP

- MSS: maximum segment size, 1460 bytes
- MTU: maximum transmission unit , 1500 bytes
- RST. SYN, FIN: used for connection setup and teardown
- PSH: to upper layer immediately. URG: urgent data, pointed by urgent data pointer
- Full-duplex: host A may receive from and send data  to server simultaneouly
- stream of bytes, not segment. segment are separated by MSS number of bytes
- ACK is receiver's next expected in-order seq
- cumulative acknowledgement: only ack bytes up to the first missing byte. 只ack连续seq
- TCP不管out-of-order segment。留给programmer处理。丢掉或者留着等之前(actually use)
- Example: Telnet, only 1 byte, for login
  - Piggybacked: receiver sends back the first byte of the segment
  - sender sends a segment with no data to indicate it has receive the ack. still has seq.
- Point-to-point

#### Three-way handshake

- client sends a first segment, server reponds a special segment, and client responds a special segment

#### Four-way handshake

- client cannot send after sent fin, but can receive



## Project 1

### Knowledge

#### Debug

- 记得改void函数时，不要丢了该return的地方！

#### Socket

- AF_INET: an address family that is used to designate the type of addresses. IPv4 网络协议的套接字类型
- SOCK_STREAM：*sock_stream* 是有保障的(即能保证数据正确传送到对方)面向连接的*SOCKET* 。数据流,一般*是tcp/ip*协议的编程。
- accept() 返回一个新的套接字来和客户端通信，addr 保存了客户端的IP地址和端口号，而 sock 是服务器端的套接字，大家注意区分。
- INADDR_ANY allows to connect to any one of the host’s IP address.

#### Fork

- pid_t waitpid(pid_t pid,int * status,int options);
  - pid<-1 等待进程组识别码为 pid 绝对值的任何子进程。
  - pid=-1 等待任何子进程,相当于 wait()。
  - pid=0 等待进程组识别码与目前进程相同的任何子进程。
  - pid>0 等待任何子进程识别码为 pid 的子进程。
- WNOHANG` 如果没有任何已经结束的子进程则马上返回, 不予以等待。
- 子进程的结束状态返回后存于 status

#### strtok

- C 库函数 **char \*strtok(char \*str, const char \*delim)** 分解字符串 **str** 为一组字符串，**delim** 为分隔符。
- 该函数返回被分解的第一个子字符串，如果没有可检索的字符串，则返回一个空指针。

```c
#include <string.h>
#include <stdio.h>
 
int main () {
   char str[80] = "This is - www.runoob.com - website";
   const char s[2] = "-";
   char *token;
   
   /* 获取第一个子字符串 */
   token = strtok(str, s);
   
   /* 继续获取其他的子字符串 */
   while( token != NULL ) {
      printf( "%s\n", token );
    
      token = strtok(NULL, s);
   }
   
   return(0);
}

```



#### Fopen

  FILE *fopen(const char *filename, const char *mode) 使用给定的模式 mode 打开 filename 所指向的文件。



#### fread

```c
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
```

- **ptr** -- 这是指向带有最小尺寸 *size\*nmemb* 字节的内存块的指针。
- **size** -- 这是要读取的每个元素的大小，以字节为单位。
- **nmemb** -- 这是元素的个数，每个元素的大小为 size 字节。
- **stream** -- 这是指向 FILE 对象的指针，该 FILE 对象指定了一个输入流。

#### fseek

```
int fseek(FILE *stream, long int offset, int whence)
```

- 设置流 **stream** 的文件位置为给定的偏移 **offset**，参数 offset 意味着从给定的 **whence** 位置查找的字节数。

- **stream** -- 这是指向 FILE 对象的指针，该 FILE 对象标识了流。

- **offset** -- 这是相对 whence 的偏移量，以字节为单位。负号前移，正号后移。

- **whence** -- 这是表示开始添加偏移 offset 的位置。它一般指定为下列常量之一：

  - | SEEK_SET | 文件的开头         |
    | -------- | ------------------ |
    | SEEK_CUR | 文件指针的当前位置 |
    | SEEK_END | 文件的末尾         |

#### ftell

```c
long int ftell(FILE *stream)
```

该函数返回位置标识符的当前值。如果发生错误，则返回 -1L，全局变量 errno 被设置为一个正值。



#### dirent.h

```c
struct dirent {
        ino_t d_ino;                    /* file number of entry */
        __uint16_t d_reclen;            /* length of this record */
        __uint8_t  d_type;              /* file type, see below */
        __uint8_t  d_namlen;            /* length of string in d_name */
        char d_name[__DARWIN_MAXNAMLEN + 1];    /* name must be no longer than this */
};

```





| a    | b    | c    |
| ---- | ---- | ---- |
| $123 | $123 | $123 |

