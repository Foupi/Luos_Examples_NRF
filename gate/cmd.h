#ifndef CMD_H
#define CMD_H

#include <json_mnger.h>
#include "stdint.h"
#include <stdbool.h>

extern volatile char detection_ask;

char *get_json_buf(void);
bool check_json(uint16_t carac_nbr);
void send_cmds(container_t *container);

#endif /* CMD_H */
