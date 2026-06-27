#ifndef SLACK_H
#define SLACK_H

// slack url
#define SLACK_URL_SEND "https://slack.com/api/chat.postMessage"
#define SLACK_URL_SOCKET_OPEN "https://slack.com/api/apps.connections.open"
#define SLACK_URL_GET_UPLOAD_URL "https://slack.com/api/files.getUploadURLExternal"
#define SLACK_URL_COMPLETE_UPLOAD "https://slack.com/api/files.completeUploadExternal"

void init_wifi();
void slack_maintain();
void slack_delay(unsigned long delay_ms);

#endif