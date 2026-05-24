//server.c

#include <termios.h>   //password masking by disabling terminal echo
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <ctype.h>

#define PORT 50284
#define MAXBUF 4096
#define LOGFILE "/srv/ie2102/IT24100284/yethmi/server_IT24100284.log"
#define USERFILE "/srv/ie2102/IT24100284/yethmi/users.db"
#define TOKENFILE "/srv/ie2102/IT24100284/yethmi/tokens.db"
#define EXPIRY 300   //5mins

// Function to disable terminal echo
void disable_echo() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~ECHO; // Disable echo
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

// Function to enable terminal echo
void enable_echo() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= ECHO; // Enable echo
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

//SIGCHLD handler
void sigchld_handler(int s) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

//Salted Hashing
void generate_salt(char *salt, size_t length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < length; i++) {
        int key = rand() % (int)(sizeof(charset) - 1);
        salt[i] = charset[key];
    }
    salt[length] = '\0';
}

void hash_password(const char *salt, const char *password, char *output) {
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", salt, password);
    unsigned long hash = 5381;
    int c;
    const char *ptr = combined;
    while ((c = *ptr++))
        hash = ((hash << 5) + hash) + c;
    sprintf(output, "%lu", hash);
}

//Logging
void log_event(const char *user, const char *cmd, const char *result, const char *ip, int port, int pid) {
    FILE *f = fopen(LOGFILE, "a");
    time_t now = time(NULL);
    fprintf(f, "[%s] IP:%s:%d PID:%d USER:%s CMD:%s RESULT:%s\n",
            strtok(ctime(&now), "\n"), ip, port, pid, user, cmd, result);
    fclose(f);
}

//Username Validation
int valid_username(const char *user) {
    int len=strlen(user);
    if(len<3 || len>20) return 0;
    for(int i=0;i<len;i++) {
        if(!isalnum(user[i])) return 0;
    }
    return 1;
}

//User Management
void add_user(const char *user, const char *salt, const char *hash) {
    FILE *f = fopen(USERFILE, "a");
    fprintf(f,"%s %s %s %d %d\n",user,salt,hash,0,0);
    fclose(f);
}

int find_user(const char *user, int *locked, int *fails, char *salt, char *stored_hash) {
    FILE *f = fopen(USERFILE,"r");
    if(!f) return 0;
    char u[50], s[50], h[65]; int l, fa;
    while(fscanf(f,"%s %s %s %d %d",u,s,h,&l,&fa)!=EOF) {
        if(strcmp(u,user)==0) {
            *locked=l; *fails=fa;
            strcpy(salt,s); strcpy(stored_hash,h);
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void update_user(const char *user, const char *salt, const char *hash, int locked, int fails) {
    FILE *f = fopen(USERFILE,"r");
    FILE *tmp = fopen("/srv/ie2102/IT24100284/yethmi/users.tmp","w");
    char u[50], s[50], h[65]; int l, fa;
    while(fscanf(f,"%s %s %s %d %d",u,s,h,&l,&fa)!=EOF) {
        if(strcmp(u,user)==0) {
            fprintf(tmp,"%s %s %s %d %d\n",user,salt,hash,locked,fails);
        } else {
            fprintf(tmp,"%s %s %s %d %d\n",u,s,h,l,fa);
        }
    }
    fclose(f); fclose(tmp);
    rename("/srv/ie2102/IT24100284/yethmi/users.tmp",USERFILE);
}

//Token management
char* create_token(const char *user) {
    static char token[50];
    sprintf(token,"TOKEN%d",rand()%10000);
    FILE *f = fopen(TOKENFILE,"a");
    fprintf(f,"%s %s %ld\n",user,token,time(NULL));
    fclose(f);
    return token;
}

int check_token(const char *token, char *user) {
    FILE *f = fopen(TOKENFILE,"r");
    if(!f) return 0;
    char u[50], t[50]; long ts;
    while(fscanf(f,"%s %s %ld",u,t,&ts)!=EOF) {
        if(strcmp(t,token)==0) {
            if(time(NULL)-ts > EXPIRY) { fclose(f); return -1; }
            strcpy(user,u);
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

//Payload Processing
void process_payload(int newfd, char *ip, int port, char *payload) {
    if(strncmp(payload,"REGISTER",8)==0) {
        char user[50], pass[50], salt[20], hash[65];
        sscanf(payload,"REGISTER %s %s",user,pass);
        if(!valid_username(user)) {
            send(newfd,"ERR 422 SID:1002 Invalid username\n",35,0);
            log_event(user,"REGISTER","INVALID",ip,port,getpid());
            return;
        }
        generate_salt(salt,8);
        hash_password(salt,pass,hash);
        add_user(user,salt,hash);
        send(newfd,"OK 201 SID:1002 Registered\n",28,0);
        log_event(user,"REGISTER","OK",ip,port,getpid());
    }
    else if(strncmp(payload,"LOGIN",5)==0) {
        char user[50], pass[50], salt[20], stored_hash[65], hash[65];
        int locked=0,fails=0;
        sscanf(payload,"LOGIN %s %s",user,pass);
        int res=find_user(user,&locked,&fails,salt,stored_hash);
        if(res==1 && !locked) {
            hash_password(salt,pass,hash);
            if(strcmp(hash,stored_hash)==0) {
                update_user(user,salt,stored_hash,0,0);
                char *token=create_token(user);
                char reply[100];
                sprintf(reply,"OK 202 SID:1002 Login successful %s\n",token);
                send(newfd,reply,strlen(reply),0);
                log_event(user,"LOGIN","OK",ip,port,getpid());
            } else {
                fails++;
                if(fails>=3) locked=1;
                update_user(user,salt,stored_hash,locked,fails);
                if(locked) send(newfd,"ERR 403 SID:1002 Account locked\n",33,0);
                else send(newfd,"ERR 401 SID:1002 Login failed\n",31,0);
                log_event(user,"LOGIN","FAILED",ip,port,getpid());
            }
        } else {
            send(newfd,"ERR 404 SID:1002 User not found\n",33,0);
            log_event(user,"LOGIN","NOTFOUND",ip,port,getpid());
        }
    }
    else if(strncmp(payload,"LOGOUT",6)==0) {
        send(newfd,"OK 203 SID:1002 Logged out\n",28,0);
        log_event("unknown","LOGOUT","OK",ip,port,getpid());
    }
    else if(strncmp(payload,"TOKEN",5)==0) {
        char token[50], user[50];
        sscanf(payload,"TOKEN %s",token);
        int res=check_token(token,user);
        if(res==1) {
            send(newfd,"OK 210 SID:1002 Token valid\n",29,0);
            log_event(user,"TOKEN","VALID",ip,port,getpid());
        } else if(res==-1) {
            send(newfd,"ERR 440 SID:1002 Session expired\n",34,0);
            log_event(user,"TOKEN","EXPIRED",ip,port,getpid());
        } else {
            send(newfd,"ERR 441 SID:1002 Invalid token\n",33,0);
            log_event("unknown","TOKEN","INVALID",ip,port,getpid());
        }
    }
    else {
        send(newfd,"ERR 400 SID:1002 Invalid command\n",34,0);
        log_event("unknown","UNKNOWN","INVALID",ip,port,getpid());
    }
}
//Client Handler
void handle_client(int newfd, char *ip, int port) {
    char buffer[MAXBUF+1];
    int bytes;
    char *ptr;

    while((bytes = recv(newfd, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes] = '\0';
        ptr = buffer;

        //Parse LEN header
        int len;
        if(sscanf(ptr, "LEN:%d\n", &len) != 1) {
            send(newfd,"ERR 400 SID:1002 Invalid length header\n",40,0);
            break;
        }

        //Find payload start
        char *payload = strchr(ptr, '\n');
        if(!payload) break;
        payload++;

        //Validate length
        if(len > MAXBUF) {
            send(newfd,"ERR 413 SID:1002 Payload too large\n",36,0);
            break;
        }
        if((int)strlen(payload) < len) {
            send(newfd,"ERR 400 SID:1002 Length mismatch\n",34,0);
            break;
        }

        //Extract payload
        char msg[MAXBUF];
        strncpy(msg, payload, len);
        msg[len] = '\0';

        //Process payload
        process_payload(newfd, ip, port, msg);
    }

    close(newfd);
}

int main() {
    int sockfd, newfd;
    struct sockaddr_in server, client;
    socklen_t addrlen = sizeof(client);

    signal(SIGCHLD, sigchld_handler);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr*)&server, sizeof(server));
    listen(sockfd, 10);

    printf("Server running on port %d\n", PORT);

    while(1) {
        newfd = accept(sockfd, (struct sockaddr*)&client, &addrlen);
        if(fork() == 0) {
            close(sockfd);
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET,&client.sin_addr,ip,sizeof(ip));
            int port = ntohs(client.sin_port);
            handle_client(newfd, ip, port);
            exit(0);
        }
        close(newfd);
    }
}
