#include "./common.h"


int main() {
	char *config = "/etc/pihealthd.conf";
	char finish[10] = {0};
	get_conf_value(config, "finish", finish);
	printf("%s\n", finish);
	return 0;
}