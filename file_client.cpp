#include "sys_cfg.h"
#include <SdFat.h>
#include "file_client.h"
#include "ether_server.h"
#include "time_client.h"
#include "vito.h"


// declare variables
SdFat sd;
SdFile file;

Sd2Card card;
FatVolume volume;

//extern char s_buf[SERVER_BUFFER_MAX_SIZE];
//extern byte s_ind;
#define f_buf s_buf
#define f_ind s_ind
#define PATH (s_buf+5)
static char old_readings[PARAM_READINGS_SIZE];
//static byte temp_str[PARAM_READINGS_SIZE];
/*****************************************************************************/
/*****************************************************************************/
uint8_t File_OpenFile(const char * fname, int oflags)
{
	if ( file.isOpen() )	file.close();
#if _DEBUG_>1
	Serial.print(F("opening file/dir: ")); Serial.print(fname);
#endif
	sd.chdir("/");	// change dir to root
	// listing of root entries
	if ( file.open(fname, oflags) ) {
#if _DEBUG_>1
		Serial.println(F(" - success."));
#endif
		return 1;
	} else {
#if _DEBUG_>1
		Serial.println(F(" - failed!"));
#endif
		sd.errorPrint();
		return 0;
	}
}
/*****************************************************************************/
/*****************************************************************************/
void File_LogError(const char * txt, byte ctrl)
{
	if ( ctrl&NEW_ENTRY )
		File_OpenFile("ERRORS.TXT", O_WRITE | O_CREAT | O_AT_END);
	if ( !file.isOpen() ) return;
	if ( ctrl&NEW_ENTRY ) {
		file.print(date_str); file.write(' '); file.print(time_str); file.print(": "); }
	if ( ctrl&P_MEM )	file.write(txt);
	else					file.print(txt);
	if ( ctrl&ADD_NL ) {
		file.println();
		file.close();
		Serial.println(F("done"));
	}
}
/*****************************************************************************/
/*****************************************************************************/
void File_LogMessage(const char * txt, byte ctrl)
{
	if ( ctrl&NEW_ENTRY ) {
			File_OpenFile("messages.txt", O_WRITE | O_CREAT | O_AT_END);
	}
	if ( !file.isOpen() ) return;
	if ( ctrl&NEW_ENTRY ) {
		file.print(date_str); file.write(' '); file.print(time_str); file.print(": "); }
	if ( ctrl&P_MEM )	file.write(txt);
	else					file.write(txt);
	if ( ctrl&ADD_NL ) {
		file.println();
		file.close();
	}
}
/*****************************************************************************/
/*****************************************************************************/
void File_LogClient(const char * txt)
{
	if ( !File_OpenFile("clients.txt", O_RDWR | O_CREAT | O_AT_END) ) return;
	file.print(date_str); file.write(' '); file.print(time_str); file.print(": ");
	file.println(txt);
	file.close();
}
/****************************************************************************
void File_LogReading(char * txt, byte ctr)
{
  if ( !File_OpenFile("READING.TXT", O_WRITE | O_TRUNC) ) return;
  if ( ctrl&NEW_ENTRY )  { file.print(date_str); file.write(' '); file.print(time_str); file.print(": "); }
  if ( ctrl&P_MEM ) file.write_P(txt);
  else         file.print(txt);
  if ( ctrl&ADD_NL ) file.println();
  file.close();
}*/
/*****************************************************************************/
/*****************************************************************************/
void File_SetDateTime(uint16_t* date, uint16_t* time)
{
  // User gets date and time from GPS or real-time clock in real callback function here
time_t t = now();
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(year(t), month(t), day(t));
  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(hour(t), minute(t), second(t));
}
/*****************************************************************************/
/*****************************************************************************/
void FileClient_Init(int CS)
{
#if _DEBUG_>0
	Serial.print(F("Initializing SD-card..."));
#endif
	//int ret = sd.begin(CS, SPI_FULL_SPEED);
	if ( !sd.begin(CS, SPI_HALF_SPEED) ) // 18MHz
		sd.initErrorHalt("failed!");

//  f_ind = 0;
	volume = *sd.vol();
#if _DEBUG_>0
	Serial.println(F("done."));
	Serial.print(F("Volume is FAT"));
	Serial.println(volume.fatType(),DEC);
#endif
	if ( !file.openRoot(&volume) )  // open root
		sd.initErrorHalt(("failed!"));
#if _DEBUG_>0
//  Serial.println(F("Root:"));
/*
DO NOT ENABLE the following line, as it causes system reset by printing out the file tree !!!
*/
	file.ls(LS_DATE | LS_SIZE);// | LS_R);	// print here the file list
	Serial.println(F("-----------end_root------------"));
#endif
	file.close();
   if ( sd.exists("logs") ) {
		File_OpenFile("logs", O_WRITE);
      // try to remove dir
      if ( !file.rmRfStar() ) {
#if _DEBUG_>0
			Serial.println(F("rmRfStar failed!"));
      } else {
			Serial.println(F("rmRfStar done."));
#endif
		}
		file.close();
	}

	sd.chdir("/");
	// set date time callback function
	SdFile::dateTimeCallback(File_SetDateTime);
	File_LogMessage(PSTR("System started."), NEW_ENTRY | ADD_NL | P_MEM); // no CLEAR | 
	old_readings[0] = 0;	// mark as invalid
}
/*****************************************************************************/
/*****************************************************************************/
void File_TestWrittenData(char * txt)
{
//  char tmp[100];
#if _DEBUG_>1
  Serial.println(F("testing written data... "));
#endif
  // open the file for write at end like the Native SD library
  int ret = File_OpenFile(f_buf, O_RDWR | O_AT_END);
  if (!ret) { sd.errorPrint("failed!"); return; }
  //Serial.print(F("file write testing: ")); Serial.println(txt);
  signed char len = strlen(txt);
  //Serial.print(F("string lenght to be tested: ")); Serial.println(len);
  int c;
  // trim right file to read back
  do {
    file.seekCur(-1);
    c = file.peek();
  } while ( (char)c<' ' );
  // trim right line to test
  while ( ((char)(c = txt[--len])<' ' ) && len>0 );
  // start here the matching
  //Serial.print(F("read chars: "));
  do {
    c = file.peek();
    //Serial.print((char)c);
    if ( (char)c!=txt[len] ) {
#if _DEBUG_>1
      Serial.print(F("\nERROR: write test failed at position ")); Serial.print(len);
      Serial.print(F(", read back: ")); Serial.println((char)c);
#endif
      // re-write the string to file
      // search last end of line
      while ( (len--)>0 )  file.seekCur(-1);
      // search last \n character
      while ( (char)(c=file.peek())!='\n' )  file.seekCur(-1);
      file.write('\n');
      file.println(txt);
      file.close();
      File_LogError(PSTR("File read back test failure, rewriting string!"), NEW_ENTRY | ADD_NL | P_MEM);
      return;
    }
    file.seekCur(-1);
  } while ( (len--)>0 );
  file.close();
#if _DEBUG_>1
  Serial.println(F("passed."));
#endif
}
/*****************************************************************************/
/*****************************************************************************/
void File_GetCurrentRecordFile(void)
{
  // bild here the file dir/name, which contains the date
  sprintf_P(f_buf, PSTR("RECORDS/%4u/%02u/%s"), year(), month(), file_str);  // "RECORDS/2015/01/15-01-23.txt"
}
/*****************************************************************************/
/*****************************************************************************/
void File_NewDay(void)
{// already done everything in the check function below
}
/*****************************************************************************/
uint8_t File_CheckMissingRecordFile(void)
{
  File_GetCurrentRecordFile();
  // check if file exists. If not, create it and write the parameter IDs
  if ( sd.exists(f_buf) ) return 0;
  // create the new file for write.
  // this will fail if case of new month, so create the new month dir first 
  while ( !File_OpenFile(f_buf, O_WRITE | O_CREAT | O_AT_END) ) {
    // check the existance of destination directory, create it if not available
    f_buf[15]=0;  // mark end of directory string by overwriting directory marker '/'
    if ( !sd.exists(f_buf) ) {
      // try to create new month dir
      if ( !sd.mkdir(f_buf, true) ) {
      // directory could not be created
#if _DEBUG_>0
        Serial.println(F("mkdir failed!"));
#endif
        File_LogError(("mkdir failed!"), NEW_ENTRY | ADD_NL | P_MEM); 
        sd.errorHalt(("mkdir failed!"));
      }
      f_buf[15] = '/';  // set back directory marker
      continue;
    }
    File_LogError(("new record file create failed!"), NEW_ENTRY | ADD_NL | P_MEM); 
    sd.errorHalt(("new record file create failed!"));
    while (1);  // halt the system, restart by watchdog
  }
  // new file created. write the parameter line
    file.print(F("Time"));  // first token
    for (byte i=0; i<7; i++) {
      signed char index = GetKeyIndex(read_params[i]);  // get the index of the parameter to log
      if ( index<0 ) break;
      file.print(",");
      file.print( Vito_GetParamName(index) );
    }
    // add EnergyCam absolute and relative values
    file.println(F(",Strom-H_abs,Strom-H_rel"));
  file.close();
  return 1;
}
/*****************************************************************************/
/*****************************************************************************/
char * GetLineParam(char * ptr, uint8_t nr) 
{
	//Serial.print(F("ptr: ")); Serial.println(ptr);
	//Serial.print(F("nr: ")); Serial.println(nr);
	// get the parameter value which is the token after the counted comma
	char * ptr1 = strtok(ptr, ",");  // token 0, is time value
	while ( (ptr1 = strtok(NULL, ","))!=NULL && (--nr)>0 );
	return ptr1;
}
/*****************************************************************************/
/*****************************************************************************/
void File_CheckDataToWrite(void)
{
	if ( old_readings[0]==0 ) {
		// copy the first line even if faulty as at this moment we don't have any better
		strcpy(old_readings, param_readings);
		return;
	}
	// check for missing parameter by detecting consecutive commas: ",,"
	byte sLen = strlen(param_readings);
	//Serial.print(F("param str length: ")); Serial.println(sLen);
	if ( strstr(param_readings, ",,")==NULL /*&& param_readings[sLen-1]!=','*/ ) {
		// all data is just fine, store this new readings
		strcpy(old_readings, param_readings);
		return;
	}
	byte oLen = strlen(old_readings);
	//Serial.println(F("< faulty string detected >"));
	char * dPtr = param_readings;  // destination
#define temp_str f_buf
	strcpy(temp_str, param_readings);
	//Serial.print(F("temp_str: ")); Serial.println(temp_str);
	char * cPtr = temp_str;  // source
	byte nr = 0;
	// data contains missing parameter(s). find its order number
	for (byte i=0; i<sLen; i++) {
		byte c = *cPtr++;  // copy character from source to destination
//		Serial.print("Char "); Serial.print(i); Serial.print(" : "); Serial.println((char)c);
		*dPtr++ = c;
		if ( c==',' ) {  // token found
			nr++;  // increment param number
			if ( (*cPtr==',' || *cPtr<' ') ) {  // next char is comma or end of string ?
				// found a missing param. copy old value to this position
				char * p1 = GetLineParam(old_readings,nr);
				if ( p1!=NULL ) {
					strcpy(dPtr, p1);  // copy the old value to the new string
					dPtr += strlen(p1);
					//Serial.print(F(".. new str: ")); Serial.println(param_readings);
				}
				// restore commas from original string
				char * p = old_readings;
				for (byte j=0; j<oLen; j++, p++)  if ( *p==0 ) *p = ',';
			}
		}
	}
}
/*****************************************************************************/
/*****************************************************************************/
void File_WriteDataToFile(void)
{
	File_CheckDataToWrite();
	pinMode(LED_PIN, OUTPUT);  // set PA0 to output
	// start of critical time interval - no power down allowed during SD writing !!!
	digitalWrite(LED_PIN, LOW);  // set LED on
	delay(1000);  // wait 1 second before writing to warn user

	TimeClient_UpdateFileString();  // update file string
	sd.chdir("/");
	File_GetCurrentRecordFile();  // init buffer with file name
	// try to open recording file
	if ( !File_OpenFile(f_buf, O_WRITE | O_AT_END) ) {
		File_LogError(("opening record file failed!"), NEW_ENTRY | ADD_NL | P_MEM); 
		sd.errorPrint(("opening record file failed!"));
		return;
	}
	file.println(param_readings);
	file.close();
#if _DEBUG_>0
	Serial.println(F("writing to file done."));
#endif
	// test if data has been correctly written.
	File_TestWrittenData(param_readings);
// end of user warning
digitalWrite(LED_PIN, HIGH);
}
/*****************************************************************************/
/*****************************************************************************/
void File_BufAdd_P(const char * str)
{
	strcpy_P(f_buf+f_ind, str);
	f_ind += strlen_P(str);
}
/*****************************************************************************/
extern void Ether_PrintSimpleHeader(EthernetClient cl);
extern void Ether_PrintStandardHeader(EthernetClient cl);
/*****************************************************************************/
// Sends a formatted HTML page containing list of files of chosen directory.
// The directory must be opened beforehand. 
/*****************************************************************************/
void File_ListDir(EthernetClient cl)
{
dir_t dir;
#if _DEBUG_>0
	Serial.println(F("listing directory."));
#endif
	// send a standard http response header
	Ether_PrintStandardHeader(cl);
	cl.println(F("<style>th,td{text-align:right;}</style>"));
	cl.println(F("<table style='width:40%'>\n<tr><th style='text-align:left'>Name</th><th>Size</th><th>Date Modified</th></tr>"));
	// This code is just copied from SdFile.cpp (lsPrintNext) of the SDFat library
	while ( file.readDir(&dir) > 0 )
	{
		// done if past last used entry
		if (dir.name[0] == DIR_NAME_FREE) break;
		// skip deleted entry and entries for . and  ..
		if (dir.name[0] == DIR_NAME_DELETED || dir.name[0] == '.') continue;
		// only list subdirectories and files
		if (!DIR_IS_FILE_OR_SUBDIR(&dir)) continue;

		f_ind = 0;
		// print any indent spaces
		File_BufAdd_P(PSTR("<tr><td style='text-align:left'><a href='"));
		for (byte i = 0; i < 11; i++) {
			if (dir.name[i] == ' ') continue;
			if (i == 8)     f_buf[f_ind++] = '.';
			f_buf[f_ind++] = (char)dir.name[i];
		}
		if ( DIR_IS_SUBDIR(&dir) )  f_buf[f_ind++] = '/';
		f_buf[f_ind++] = '\''; f_buf[f_ind++] = '>';
		// print file name with possible blank fill
		for (byte i = 0; i < 11; i++) {
			if (dir.name[i] == ' ') continue;
			if (i == 8)  f_buf[f_ind++] = '.';
			f_buf[f_ind++] = (char)dir.name[i];
		}
		if ( DIR_IS_SUBDIR(&dir) )  f_buf[f_ind++] = '/';
		File_BufAdd_P(PSTR("</a></td><td>"));
		// next cell is the file size
		if ( DIR_IS_SUBDIR(&dir) ) {
			File_BufAdd_P(PSTR(" - "));
		} else { //if ( flags & LS_SIZE ) // print size if requested
			sprintf_P(f_buf+f_ind, PSTR("%6lu"), (long int)dir.fileSize); f_ind += 6;
		}
		//client.print(F("</td><td>"));
		File_BufAdd_P(PSTR("</td><td>"));
		// write next cell: print modify date/time
		sprintf_P(f_buf+f_ind, PSTR("%4u-%02u-%02u %02u:%02u:%02u"),
			int(FAT_YEAR(dir.lastWriteDate)), FAT_MONTH(dir.lastWriteDate), FAT_DAY(dir.lastWriteDate),
			FAT_HOUR(dir.lastWriteTime), FAT_MINUTE(dir.lastWriteTime), FAT_SECOND(dir.lastWriteTime));
		f_ind += 19;  // the length of date and time
		//client.println(F("</td></tr>"));
		File_BufAdd_P(PSTR("</td></tr>\n"));
		// send here the table line string to ethernet client
		cl.print(f_buf);
	}
  cl.println(F("</table>\n</html>"));	// table and file closing markers
#if _DEBUG_>1
//  Serial.println(F(" done."));
#endif
	file.close();  // close root.
}
/*****************************************************************************/
/* Send the list of record files of the current month
/*****************************************************************************/
void File_SendFileList(EthernetClient cl)
{
dir_t dir;
	// send reply header
	Ether_PrintSimpleHeader(cl);
#if _DEBUG_>1
//  Serial.print(F("send list dir ..."));
#endif
	// This code is just copied from SdFile.cpp (lsPrintNext) of the SDFat library
	while (file.readDir(&dir) > 0)
	{
		// done if past last used entry
		if (dir.name[0] == DIR_NAME_FREE) break;
		// skip deleted entry and entries for . and  ..
		if (dir.name[0] == DIR_NAME_DELETED || dir.name[0] == '.') continue;
		// only list files
		if ( DIR_IS_SUBDIR(&dir) ) continue;

		f_ind = 100; // temporary storage for file name - preserve lower order bytes of the buffer 
		// print any indent spaces
		for (byte i = 0; i < 11; i++) {
			if (dir.name[i] == ' ') continue;
			if (i == 8)     f_buf[f_ind++] = '.';
			f_buf[f_ind++] = (char)dir.name[i];
		}
		f_buf[f_ind] = 0;  // end of string
		// send here the filename line string to ethernet client
		cl.println(f_buf+100);
	}
	f_buf[0] = 0;  // avoid sending elapsed time footer
#if _DEBUG_>1
//  Serial.println(F(" done."));
#endif
	file.close();  // close root.
}
/*****************************************************************************/
void File_LoadFileLine(void)
{
  #define MINIMUM_BYTES 50
  // loads a minimum of 50 bytes to increase transmission efficiency
  f_buf[0] = 0;
  if ( file.fgets(f_buf, MINIMUM_BYTES, (char*)"")==MINIMUM_BYTES ) {
    file.fgets(f_buf+MINIMUM_BYTES, SERVER_BUFFER_MAX_SIZE-MINIMUM_BYTES, (char*)"\n");
  }
}
//-----------------------------------------------------------------------------
void File_SendFile(EthernetClient cl)
{
	// check file type and add the corresponding description to the HTTP header
	if ( strstr(PATH, (".png"))!=NULL ) {
		cl.println(F("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\n"));
	} else
	if ( strstr(PATH, (".jpg"))!=NULL ) {
		cl.println(F("HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\n"));
	} else
	if ( strstr(PATH, (".bmp"))!=NULL ) {
		cl.println(F("HTTP/1.1 200 OK\r\nContent-Type: image/bmp\r\n"));
	} else
	if ( strstr(PATH, (".gif"))!=NULL ) {
		cl.println(F("HTTP/1.1 200 OK\r\nContent-Type: image/gif\r\n"));
	} else
	if ( strstr(f_buf,(".htm"))!=NULL )
		cl.println(F("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"));
	else {
		cl.println(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\n"));
		// send in ASCII mode files: txt, js, htm, 
		while (1) {
			File_LoadFileLine(); // ascii mode
			if ( f_buf[0]==0 ) break;
			cl.print(f_buf);
		}
		return;
	}
	// sending data in binary mode
	byte c;
	while ( (c = file.read(f_buf, sizeof(f_buf)-1))> 0 ) {
		cl.write(f_buf, c);
	}
	file.close();  // close root.
	f_buf[0] = 0;  // don't send elapsed time
}
/*****************************************************************************/
/*****************************************************************************/
void File_CheckPostRequest(EthernetClient cl)
{
char * f_ptr;
	if ( strstr(PATH, ("getfilelist="))!=NULL ) {
#if _DEBUG_>0
		Serial.println(F("sending file list."));
#endif
		// get the list of files of current month. prepare the directory for current month
		sprintf(PATH, ("records/%4u/%02u"), year(), month());
		if ( File_OpenFile(PATH, O_READ) )
			File_SendFileList(cl);
	} else
	if ( ( f_ptr = strstr(PATH, ("readfile=")) )!=NULL ) {  //14-12-30.TXT)
		// send requested record file. check if file exists
#if _DEBUG_>0
		Serial.println(F("sending file."));
#endif
		// backup txt file
		strcpy(f_buf+90, f_ptr+9);	// the file name already ends in a '0'
		f_ptr = f_buf+90;	// set new pointer
		// prepare requested file with correct directory
		sprintf(PATH, ("records/20%2u/%02u/%s"), atoi(f_ptr), atoi(f_ptr+3), f_ptr);
		if ( File_OpenFile(PATH, O_READ) )
			File_SendFile(cl);
	}
	file.close();
}
/*****************************************************************************/
/*****************************************************************************/
char * String_Trim(char * inPtr)
{
	while ( *inPtr <= ' ' )	inPtr++;	// goto first valid char
	char * retPtr = inPtr;	// store pointer
	while ( *inPtr > ' ' )	inPtr++;	// goto end of string
	inPtr = 0;	// mark end of string
	if ( *(--inPtr) == '/' )
		*inPtr = 0; // remove '/' from the end
	return retPtr;
}
/*****************************************************************************/
/*****************************************************************************/
void File_CheckDirFile(EthernetClient cl)
{
byte index = 0;
   char * f_ptr = PATH;
#if _DEBUG_>0
	uint32_t tim = millis();
	// print the file we want 
	Serial.print(F("Checking request: ")); Serial.println(PATH);
#endif
	// check start page. If nothing specified, then the index pages should be redirected.
	if ( *f_ptr==0 ) {
		cl.println(F("HTTP/1.1 301 Moved Permanently\r\nLocation: web/index.htm\r\n"));
		return;   // wait for next request with correct path
	}
	if ( strstr(PATH, ("/index.htm"))!=NULL )
		index = 1;
	// remove leading "files" if it is there
	if ( strncmp(PATH, ("files"), 5)==0 )
		f_ptr = PATH + 5;  // end of: /files^
	else
		f_ptr = PATH;
	// check starting dir. If nothing specified, then the root file list should be transmitted.
	if ( *f_ptr==0 )   {
		*f_ptr = '/';
		*(f_ptr+1) = 0;
	}
	// check if requested file/dir exists
	if ( File_OpenFile(f_ptr, O_READ) ) {
		if ( file.isDir() || file.isSubDir() ) {  // root or directory?
			File_ListDir(cl);
		} else {
			File_SendFile(cl);
			file.close();  // close root.
			if ( index ) {
				// send here the additional data
				cl.print(F("<div class='myp'>\n<p id='crtDateTime'>"));
				sprintf_P(s_buf, PSTR("%s, 20%s, %s"), dayStr(weekday()), date_str, time_str);
				cl.print(s_buf);
				cl.print(F("</p>\n<p id='params'>"));
				File_GetRecordLine(0);
				cl.print(s_buf);
				cl.print(F("</p>\n<p id='values'>"));
				File_GetRecordLine(-1);
				cl.print(s_buf);
				cl.println(F("</p>\n</div>\n<script>ParseData();</script>"));
			}
		}
		file.close();  // close root.
	} else {
		cl.println(F("HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n"));
		cl.print(PATH);
		cl.println(F(" - not found!"));
	}

#if _DEBUG_>0
  Serial.print(F("...processed in: ")); Serial.println(millis()-tim);
#endif
	//sd.chdir("/");
}
/****************************************************************************/
void File_GetFileLine(int line_nr)
{
//  File_GetCurrentRecordFile();  // returns the file name in f_buf
	if ( !File_OpenFile(f_buf, O_READ) ) {
		File_LogError(PSTR("Could not open file: "), NEW_ENTRY | P_MEM);
		File_LogError(f_buf, ADD_NL);
		f_buf[0] = 0;
		return;
	}
	// file opened successfully
	memset(f_buf, 0, sizeof(f_buf));  // init read buffer
	int line = 0;
	if ( line_nr>=0 ) {	// read from the beginning
		file.rewind();  // set to position zero
	} else {  // read last stored line, from the end
#if _DEBUG_>0
//	Serial.println(F("requested last line..."));
#endif
		file.seekEnd();  // set to end of file
		// trim right file to read back
		while ( file.seekCur(-1) && (char)(file.peek())<' ' );
		// find next end of line
		while ( file.seekCur(-1) && (char)(file.peek())>=' ' );
		file.seekCur(+1); // go to the next valid char
		//Serial.print(F("read chars: "));
		line = line_nr;	// read only one line, the last one
	}
	// read characters untill end of line reached:
	while ( 1 ) {
		int fg = file.fgets(f_buf, sizeof(f_buf)-1);
//    Serial.println(fg);
		if ( fg<=0 ) {
			f_buf[0] = 0;
			break;  // we are done
		}
		if ( line == line_nr ) break;  // we are done
		f_buf[0] = 0;	// mark invalid string
		line ++;
	}
	file.close();
	// the buffer is ready to be read
}
/****************************************************************************/
void File_GetRecordLine(int line_nr)
{
	File_GetCurrentRecordFile();  // current log file name is stored in f_buf
	File_GetFileLine(line_nr);  // the input file name is in f_buf, the read line is returned also in f_buf
}
/****************************************************************************/
char * File_GetRecordedParameter(int line_nr)
{
	// get position of the parameter in the log line.
	// read the first line of the reading log file
	File_GetRecordLine(0);
	byte i = 1;
	// count the separating markers ',' in the string before the input param name
	char * ptr = strtok(f_buf, ",");
	while ( (ptr = strtok(NULL, ","))!=NULL && strcmp(ptr, param_name)!=0 )	i++;
	//Serial.println(ptr);
	if ( ptr==0 ) return 0;
	File_GetRecordLine(line_nr);
	//Serial.print(F("Read recorded line: ")); Serial.print(f_buf);
	if ( f_buf[0]==0 ) return 0;  // line number was not found
	// get the parameter value which is the token after the counted comma
	return GetLineParam(f_buf, i);
}
