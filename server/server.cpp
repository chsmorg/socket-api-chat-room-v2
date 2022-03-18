//
//
// server chat room projet v2
// server allows for multiple clients to connect and send commands that are displayed in the server window as well as the client window
// 
// chase morgan
// created on 3/15/2022
//




#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "winsock2.h"
#include <thread>
#include <vector>
#include <string>





#define SERVER_PORT   13388
#define MAX_PENDING   5
#define MAX_LINE      290 // chenged to allow for messages to to contain at least 256 characters 
#define MAX_CONNECTED  3

std::vector<std::unique_ptr<std::thread>> threads;  //holds threads
int connected = 0; //connected clients
int loginCount = 0;  //logged in clients 
char connectedUsers[MAX_CONNECTED][33];  //username array for logged in users
std::vector<SOCKET> userSockets;  //socket vector



//login function 
int login(char* username, char* password) {  
    char usernames[MAX_LINE][32];   //2d array for usernames and passwords allows for 235 users
    char passwords[MAX_LINE][8];  
    int len;

    FILE* fp = fopen("users.txt", "r");  //opens users file
    if (!fp) {
        printf("Error connecting to users list.\n");  
            return 0;
    }
    for (len=0; !feof(fp); len++) {
        fscanf(fp, "%s %s\n", usernames[len], passwords[len]);     //gets each entry from file and put them into arrays
    }
    fclose(fp);

    for (int i = 0; i < len; i++) {         //loops through the arrays tokening out the useless characters and checking if the client inputs match an entry in the file
        if (strcmp(username, strtok(usernames[i],"(),")) == 0 && strcmp(password, strtok(passwords[i], "( ) , ")) == 0) { 
            return 1;      
        }
        else if (strcmp(username, strtok(usernames[i], "(),")) == 0) {  //checks if username is right but password is not
            return 2;
        }
    }
    return 0; 
}
//new user function
int newuser(char* username, char* password) {
    FILE* fp = fopen("users.txt", "a"); //opens uesrs file
    if (!fp) {
        printf("Error connecting to users list.\n"); //file error
        return -1;
    }
    int l = login(username, password);
    if (l == 1) {  //checks if client username and password are already in the file returns 0 if it does
        fclose(fp);
        return 0;
    }
    else if (l == 2) {  //checks if username is taken returns 2 if it is
        fclose(fp);
        return 2;
    }
    else {

            fprintf(fp, "\n(%s, %s)", username, password); //writes new entry into file
            fclose(fp);
            return 1;
    }
    fclose(fp);
    return -1;
}

void removeUser(char array[][33], char* username) { //removes username from username list
    int pos = 0;

    for (pos = 0; pos < loginCount; pos++) {
        if (strcmp(array[pos], username) == 0) {  //locates username postion
            if (pos == loginCount - 1) {  //if it is located at the end, return
                loginCount--;
                return;
            }

            for (int i = 0; i < loginCount; i++) {  // otherwise go to postion of user loggin out and move all other users down one postion
                if (pos == i) {
                    strcpy(array[pos], connectedUsers[pos+1]);
                    pos++;
                }
            }
            
            loginCount--; //dec login count
            return;
        }
    }
  
    
    return;
    
}
void removeSocket(SOCKET s) {  //removes a socket from the socket list
    for (int i = 0; i > userSockets.size(); i++) {
        if (s == userSockets.at(i)) {
            std::vector<SOCKET>::iterator it1 = userSockets.begin() + i;  //locate postion of socket in list

            std::vector<SOCKET>::iterator it2 = userSockets.begin() + i + 1;
            
            userSockets.erase(it1,it2);  //remove socket from vector
            break;
        }
    }
}

void sendToAll(char* message) { // sends to all users in chat
    for (SOCKET s : userSockets) {
        send(s, message, strlen(message), 0);   //for each socket, send message over socket
    }
    return;
}
void sendToAllButUser(char* message, SOCKET us) {  //sends to all sockets but the sender
    for (SOCKET s : userSockets) {    
        if (us == s) continue;    //loops through vector and sends message over sockets unless the socker is the senders
        send(s, message, strlen(message), 0);
    }
    return;
}

//command not working 
void sendToUser(char* username, char* message){  //sends message to a userID
    int pos = 0;

    for (pos = 0; pos < loginCount; pos++) {   
        if (strcmp(connectedUsers[pos], username) == 0) {  //locates postion of userId in userid list
            send(userSockets.at(pos), message, strlen(message), 0);  //sends to socket at the same postion of userid
            break;
        }
        //does not work becuase socket list is based off when user connects, userid list is based off when user logs in
    }
    return;
}





class CLientThread {  //thread to run each client independently
public:
    SOCKET socket;
    
    void operator()(SOCKET s) {
        char username[32];
        char buf[MAX_LINE];
        char input[MAX_LINE];
        socket = s;
        bool loggedin = false;
        int len;

            while(1) {
                
                char buf[MAX_LINE];
                char m[MAX_LINE] = "\0";                                               //placeholder for message to be sent to clients(s)
                int len = recv(s, buf, MAX_LINE, 0);   //receives clinet message associated with socket upon creation

                if (len == -1) {  //if client dissconnects without using logout
                    removeSocket(s);  //removes both socket and user from lists
                    closesocket(s);
                    connected--;
                    if (loggedin) {
                        removeUser(connectedUsers, username);
                    }
                    break;                                                         
                }

                buf[len] = 0;
                strtok(buf, " ");                                            //tokens the buffer to get the first word from the client input

                if (strcmp(buf, "login") == 0) {                           //if input is login and no user is logged in 
                    if (loggedin == false) {


                        char* u = strtok(NULL, " ");
                        char* p = strtok(NULL, " ");  //tokens the username and password form input
                        int lin = login(u, p);       //checks if the user is able to login with the input
                        if (lin == 0 || lin == 2) {              //if check fails, set message to error message and send to client
                            strncpy(m, "Denied. User name or password incorrect.", sizeof(m));

                        }
                        else {
                            loggedin = true;    //if user exists in file, set login to true and send a confirmation message to sever and client
                            char ma[50];        //placeholder for join message to be set out to other users
                            strncpy(m, "login confirmed", sizeof(m));
                            strcpy(username, u);
                            strcpy(connectedUsers[loginCount], u);  //adds username to userid list
                            loginCount++;
                            printf("%s login.\n", username);
                            strcpy(ma, username);
                            strncat(ma, " joins.", 8);
                            sendToAllButUser(ma, socket);  //sends message to server chat

                        }
                    }
                    else {             //otherwise send an error message to client if they are already logged in

                        strncpy(m, "already logged in.", sizeof(m));


                    }

                }
                else if (strcmp(buf, "logout") == 0) {    //if logout and logged in
                    if (loggedin == true) {
                        
                        


                        printf("%s logout.\n", username);
                        strncat(username, " left", 5);
                        sendToAll(username);              //send logout message to clients and server 
                        removeUser(connectedUsers, username); //removes user from userId list
                        
                        
                               //send logout message to client and server 

                        loggedin = false;
                        
                        removeSocket(s);    //remove socket from list
                        closesocket(s);      //close the client socket and break from the loop
                        connected--;
                        break;
                    }
                    else {          //if not logged in, send diffrent error message and close the socket

                        strncpy(m, "Not logged in. exiting", sizeof(m));
                        send(s, m, strlen(m), 0);
                        connected--;
                        removeSocket(s);  //removes socket from list
                        closesocket(s);
                        break;  //break from loop

                    }
                }
                else if (strcmp(buf, "newuser") == 0) { //if newuser
                    if (!loggedin) {

                        char* u = strtok(NULL, " ");
                        char* p = strtok(NULL, " ");  //token username and password from client input
                        switch (newuser(u, p)) {       //validate if user can be created and send error messages accordingly

                        case 0:
                            strncpy(m, "Denied. User account already exists. ", sizeof(m));
                            break;

                        case 1:
                            strncpy(m, "New user account created. Please login.", sizeof(m));  //prints to server if new user is created
                            printf("New user account created.\n");
                            break;
                        case 2:
                            strncpy(m, "user already exists, try again", sizeof(m));  //prints to server if new user already exists 
                            break;
                        default:
                            strncpy(m, "User creation failed. Try again.", sizeof(m));
                            break;

                        }
                    }
                    else {
                        strncpy(m, "Please log out to create a new user", sizeof(m));  //error is user is logged in
                    }
                }
                else if (strcmp(buf, "send") == 0) {  //if send
                    if (loggedin) {
                        char* id = strtok(NULL, " ");
                        char* message = strtok(NULL, "");  //tokens anything after the command into message
                        if (strcmp(id, "all") == 0) {
                            strcpy(m, username);                    //copies username into m
                            strncat(m, ": ", 1);                  //adds a : after username
                            strncat(m, message, strlen(message));   //adds the message sent by the client to m
                            printf("%s\n", m);  //prints result to server
                            sendToAllButUser(m, socket);   //sends the message to all other users on server
                            continue;
                        }
                        
                        


                    }
                    else {
                        strncpy(m, "Denied. Please login first.", sizeof(m));  //error messsage if client is not logged in
                    }
                }
                else if (strcmp(buf, "who") == 0) {

                    if (loggedin) {
                        strcpy(m, connectedUsers[0]);
                        
                        for (int i = 1; i < loginCount; i++) {  //loops through loggedin users adding them to the message
                            strncat(m, ", ", 2);
                            strncat(m, connectedUsers[i], strlen(connectedUsers[i]));  
                            

                        }
                    }
                    else {
                        strcpy(m, "login to use this command");
                    }

                    
                        
                        
                    
                }
                send(s, m, strlen(m), 0); // sends resulting message to client  and repeats loop while client is still logged in
              
            }
            
    }
};

   void main() {

       int opt = true;
   
    // Initialize Winsock.
      WSADATA wsaData;
      int iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
      if ( iResult != NO_ERROR ){
         printf("Error at WSAStartup()\n");
         return;
      }
   
    // Create a socket.
      SOCKET listenSocket;
      listenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
   
      if ( listenSocket == INVALID_SOCKET ) {
         printf( "Error at socket(): %ld\n", WSAGetLastError() );
         WSACleanup();
         return;
      }

      //allows for multiple connections

      if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt,
          sizeof(opt)) < 0)
      {
          printf("opt failed.\n");
          closesocket(listenSocket);
          WSACleanup();
          return;
      }

   
    // Bind the socket.
      sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = INADDR_ANY; //use local address
      addr.sin_port = htons(SERVER_PORT);
      if ( bind( listenSocket, (SOCKADDR*) &addr, sizeof(addr) ) == SOCKET_ERROR ) {
         printf( "bind() failed.\n" );
         closesocket(listenSocket);
         WSACleanup();
         return;
      }
   
    // Listen on the Socket.
      if ( listen( listenSocket, MAX_PENDING ) == SOCKET_ERROR ){
         printf( "Error listening on socket.\n");
         closesocket(listenSocket);
         WSACleanup();
         return;
      }
   
    // Accept connections.
      SOCKET s;
      int size = sizeof(addr);
      //int addrlen;
      
   
      printf( "My chat room server. Version Two.\n\n" );
      
      while(1){

          s = accept(listenSocket, (struct sockaddr*)&addr, &size); //accepts new connection
          if (s == SOCKET_ERROR) {
              printf("accept() error \n");
              closesocket(listenSocket);
              WSACleanup();
              return;
          }

          if (connected < MAX_CONNECTED) {  //allows for 3 users to join the server at a time
   

              threads.emplace_back(new std::thread(CLientThread(), s)); // creates a new thread for each new socket and adds them to a list
              userSockets.emplace_back(s);  //adds socket to a list
              connected++;


             
          }
          else {
              send(s, "Chat server is full, try again later.", 30, 0); //sends error message if server is full
              closesocket(s);
          }
         
      }
   
      closesocket(listenSocket);
   }

   
  
             


