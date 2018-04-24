#ifndef _CONSTANT_H
#define _CONSTANT_H

static const char* SERVER_CONNECTED_MESSAGE = "220 Server is connected.\n";
static const char* SUCCESS_LOGIN = "230 User has successfully logged in.\n";
static const char* SUCCESS_QUIT = "221 Successfully closing control connection.\n";
static const char* SUCCESS_DIRECTORY = "250 Succesfully changed directory.\n";
static const char* ASCII_COMMAND = "200 Type switched to ASCII.\n";
static const char* IMAGE_COMMAND = "200 Type switched to IMAGE.\n";
static const char* STREAM_COMMAND = "200 Mode switched to STREAM.\n"; 
static const char* FILE_COMMAND = "200 Stru switched to FILE.\n";

static const char* UNSUPPORTED_COMMAND = "500 Command is not supported.\n";
static const char* INCORRECT_ARGUEMENTS = "501 Incorrect number of arguments, or invalid arguments.\n";
static const char* UNABLE_LOGIN = "530 Unable to login.\n";
static const char* PLEASE_LOGIN = "530 Please login.\n";
static const char* INVALID_DIRECTORY = "550 Invalid directory.\n";
static const char* UNIMPLEMENTED_COMMAND = "504 Command is not implemented.\n";
static const char* RETR_FAIL = "550 Failed to open file.\n";

static const char* SUCCESS_PASSIVE = "227 Entering Passive Mode (%s,%s,%s)\n";
static const char* REQUIRE_PASSIVE = "425 Enter passive mode first.\n";
static const char* TIMEOUT_PASSIVE = "425 Data connection timeout.\n";

static const char* INCOMING_DIRECTORY_LISTING = "150 Incoming directory listing.\n";
static const char* DIRECTORY_LISTING_SUCCESS = "226 Directory successfully sent.\n";
static const char* DIRECTORY_DOES_NOT_EXIT = "451 The named directory does not exist or you don't have permission to read it.\n";
static const char* NOT_ENOUGH_RESOURCES = "426 Insufficient resources to perform request.\n";
static const char* INCOMING_FILE = "150 Opening BINARY mode data connection for %s (%d bytes).\n";
static const char* TRANSFER_COMPLETE = "226 Transfer Complete.\n";


static const char* DOT_SLASH = "./";
static const char* DOT_DOT_SLASH = "../";

#endif
