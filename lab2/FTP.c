typedef struct url {
	char * user;
	char * password;
	char * host;
	char * url_path;
} url;

void infoStruct(char * urlstr, url * info){
	char * temp = malloc(urlstr);
	strcpy(temp,urlstr);

	info->user = calloc(strlen(temp), sizeof(char));
	info->password = calloc(strlen(temp), sizeof(char));
	info->host = calloc(strlen(temp), sizeof(char));
	info->url_path = calloc(strlen(temp), sizeof(char));

	temp = strtok(temp+6, ":");
	strcpy(info->user, temp);
	temp = strtok(NULL, "@");
	strcpy(info->password, temp);
	temp = strtok(NULL, "/");
	strcpy(info->host, temp);
	temp = strtok(NULL, "");
	strcpy(info->url_path, temp);


}

int main(int argc, char** argv){

	if (argc < 2){
		printf("Usage:\tdownload ftp://[<user>:<password>@]<host>/<url-path>\n");
		exit(1);
	}

	url *URL_INFO  malloc(sizeof(url));

	infoStruct(argv[1], URL_INFO);


	//clientTCP.c

	//connectServer(socketfd, res);

	//readFromServer(socketfd, answer);
}