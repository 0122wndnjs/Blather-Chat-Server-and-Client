#include "blather.h"

client_t client_actual = {}; //client as global variable
client_t *client = &client_actual; //pointer to client

simpio_t simpio_actual; //simpio as global variable
simpio_t *simpio = &simpio_actual; //pointer to simpio

pthread_t user_thread; //user interaction thread(user input)
pthread_t server_thread; //server interaction thread(reading mesg from server)

int join_fd; //fd for join fifo to server

void *user_worker(void *arg){ //function passed into user thread

  while(!simpio->end_of_input){ // repeat until End of Input
    simpio_reset(simpio); //reset simpio every time getting new input
    iprintf(simpio, "");
    while(!simpio->line_ready && !simpio->end_of_input){ //get input by one char
      simpio_get_char(simpio);
    }
    if(simpio->line_ready){ //if input is complete

      mesg_t mesg_actual = {}; //mesg
      mesg_t *mesg = &mesg_actual; //pointer to mesg

      mesg->kind = BL_MESG; //set kind as BL_MESG
      strncpy(mesg->name, client->name, MAXPATH); //copy name to mesg
      strncpy(mesg->body, simpio->buf, MAXPATH); //copy body(text) to mesg

      // iprintf(simpio, "kind, name and body: %d %s %s ", mesg->kind, mesg->name, mesg->body);

      int res = write(client->to_server_fd, mesg, sizeof(mesg_t)); //send data to server
      check_fail(res==-1,1,"Sending msg to server Fail!\n");
    }
  }

  mesg_t mesg_actual = {}; //mesg
  mesg_t *mesg = &mesg_actual; //pointer to mesg

  //if client entered End of Input (means client departed)
  mesg->kind = BL_DEPARTED;
  snprintf(mesg->name, MAXNAME, "%s", client->name);
  write(client->to_server_fd, mesg, sizeof(mesg_t)); //send departure message to server

  pthread_cancel(server_thread); //cancel server thread since user departed chat
  return NULL;

}

void *server_worker(void *arg){

  mesg_t mesg_actual; //mesg
  mesg_t *mesg = &mesg_actual; //pointer to mesg
  mesg->kind = BL_MESG; // initialize kind to default
  // int res = read(client->to_client_fd, mesg, sizeof(mesg_t)); //get message first
  // printf("read: %d\n", res);

  while(mesg->kind != BL_SHUTDOWN){ //repeat till getting shutdown mesg from server

    int res = read(client->to_client_fd, mesg, sizeof(mesg_t)); //get message first
    check_fail(res==-1,1,"Read from server Fail!\n"); // error check

    switch(mesg->kind){ // do jobs based on mesg kind
      case BL_MESG :
        iprintf(simpio, "[%s] : %s\n", mesg->name, mesg->body);
        break;
      case BL_JOINED :
        iprintf(simpio, "-- %s JOINED --\n", mesg->name);
        break;
      case BL_DEPARTED :
        if(!strcmp(mesg->name,client->name)){
          break; //the iprintf should not show up to the client who is actually departing..
        }
        iprintf(simpio, "-- %s DEPARTED --\n", mesg->name);
        break;
      default :
        break;
    }

  }

  iprintf(simpio, "!!! server is shutting down !!!\n"); // let know that server is shutting down

  pthread_cancel(user_thread); //cancel user thread for server is shutting down
  return NULL;

}

int main(int argc, char *argv[]){

  if(argc < 3){ //if user did not put command args properly
    printf("usage: %s <server name> <client name>\n", argv[0]);
    exit(1);
  }

  char server_name[MAXPATH]; //server name
  char join_fifo[MAXPATH]; //join fifo name
  snprintf(server_name, MAXPATH, "%s", argv[1]);
  snprintf(join_fifo, MAXPATH, "%s.fifo", argv[1]);

  snprintf(client->name, MAXPATH, "%s", argv[2]); //client name
  snprintf(client->to_client_fname, MAXPATH, "%d.tc.fifo", getpid()); //to-client fifo name
  snprintf(client->to_server_fname, MAXPATH, "%d.ts.fifo", getpid()); //to-server fifo name
  client->data_ready = 0; //data is not ready so 0

  join_t join_req = {}; //join request to server
  snprintf(join_req.name, MAXPATH, "%s", client->name); //copy name to request
  // printf("name is %s\n", join_req.name);
  snprintf(join_req.to_client_fname, MAXPATH, "%s", client->to_client_fname); //copy fifo name to request
  snprintf(join_req.to_server_fname, MAXPATH, "%s", client->to_server_fname); //copy fifo name to request

  remove(client->to_client_fname); //remove previously existing fifo
  remove(client->to_server_fname); //remove previously existing fifo
  mkfifo(client->to_client_fname, DEFAULT_PERMS); //make fifo
  mkfifo(client->to_server_fname, DEFAULT_PERMS); //make fifo
  client->to_client_fd = open(client->to_client_fname, O_RDWR); //open to-client fd
  client->to_server_fd = open(client->to_server_fname, O_RDWR); //open to-server fd
  join_fd = open(join_fifo, O_RDWR); //open fd for join fifo

  //now server knows the client and they can interact!

  write(join_fd, &join_req, sizeof(join_t));

  char prompt[MAXNAME]; // prompt buffer
  snprintf(prompt, MAXNAME, "%s>> ", client->name); // create a prompt string
  simpio_set_prompt(simpio, prompt);         // set the prompt
  simpio_reset(simpio);                      // initialize simpio
  simpio_noncanonical_terminal_mode();       // set the terminal into a compatible mode

  pthread_create(&user_thread,   NULL, user_worker,   NULL);     // start user thread to read input
  pthread_create(&server_thread, NULL, server_worker, NULL);    // start server thread to read message from server

  pthread_join(user_thread, NULL);      // wait for user thread to return
  pthread_join(server_thread, NULL);    //wait for server thread to return



  simpio_reset_terminal_mode(); // reset terminal at end of run
  printf("\n");

  return 0;


}
