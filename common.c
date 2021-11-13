
int send_trama(int fd, char * buf, int length){
  int i = 0;
  while(i < length){
    write(fd,&buf[i],1);
    i++;
  }
}