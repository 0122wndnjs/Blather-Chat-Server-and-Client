#include "blather.h"

int signaled = 0;

void handle_signals(int signo){
  signaled = 1;
  return;
}

int main(int argc, char *argv[]){

  struct sigaction my_sa = {};
  my_sa.sa_handler = handle_signals;
  sigemptyset(&my_sa.sa_mask);
  my_sa.sa_flags = SA_RESTART;
  sigaction(SIGTERM, &my_sa, NULL); //set sigterm signal handler
  sigaction(SIGINT, &my_sa, NULL); //set sigint signal handler

  if(argc < 2){
    printf("usage: %s <server name>\n", argv[0]);
    exit(1);
  }

  server_t server_actual;
  server_t *server = &server_actual;
  server_start(server, argv[1], DEFAULT_PERMS);

  while(!signaled){
    server_check_sources(server);
    if(server_join_ready(server)){
      server_handle_join(server);
    }
    for(int i = 0; i < server->n_clients; i++){
      if(server_client_ready(server, i)){
        server_handle_client(server, i);
      }
    }
  }


  server_shutdown(server);
  exit(0);



}
