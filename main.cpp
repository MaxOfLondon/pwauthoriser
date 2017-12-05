#include <windows.h>
#include "resource.h"
#include <iostream>
#include <sstream>
#include <string>
#include <stddef.h>
#include <limits.h>
#include <bitset>
#include <ctime>
#include <tchar.h>
#include "Tokenizer.h"
#include <list>
#include <vector>
#include <sqlite3.h>
#include "Database.h"
#include "Query.h"
#include <sys/stat.h>

//using namespace std;

struct DATA {
	ULONG day;		// 1-31 (5bits)
	ULONG month;	// 0-11 (4bits)
	ULONG year; 	// 0-63 (6bits)
	ULONG minute;	// 0-59 (6bits)
	ULONG hour;		// 1-24 (5bits)
	ULONG userid;	// 0-1024 (10bits)
	ULONG ref;		// 0-268435456 (28bits)
	string dateStr;
}; 

struct USER {
	ULONG id;
	string login;
	string firstname;
	string lastname;
	string department;
	int priv;
};

typedef DATA* PDATA;
typedef bitset<64> BITSET;
typedef BITSET* PBITSET;

HINSTANCE TheInstance = 0;
// conversion table used to encrypt (swap) bits in the bitset
const byte CONV[] = {0x0C, 0x2D, 0x32, 0x38, 0x15, 0x29, 0x00, 0x19, 0x0B, 0x16, 0x3B, 0x35, 0x18, 0x34, 0x12, 0x3A, 
					0x05, 0x25, 0x21, 0x01, 0x1E, 0x2B, 0x26, 0x03, 0x0D, 0x1F, 0x27, 0x1B, 0x09, 0x0E, 0x14, 0x30, 
					0x3F, 0x36, 0x02, 0x22, 0x0A, 0x0F, 0x1A, 0x28, 0x06, 0x24, 0x33, 0x1C, 0x3E, 0x39, 0x1D, 0x3D, 
					0x04, 0x11, 0x31, 0x2F, 0x2C, 0x17, 0x23, 0x20, 0x2E, 0x37, 0x07, 0x2A, 0x08, 0x13, 0x10, 0x3C
					};

const char* PHWORDS[] = {"zero", "one", "two", "three", "four", "five", "six",
					"seven", "eight", "nine", "", "", "", "", "", "", "",
					"alpha", "bravo", "charlie", "delta", 
					"echo", "foxtrot", "golf", "hotel", "india", "juliet", 
					"kilo", "lima", "mike", "november", "oscar", "papa", 
					"quebec", "romeo", "sierra", "tango", "uniform", "victor",
					"whisky", "x-ray", "yankee", "zulu"};


BITSET   bs(0);
Database db("db.db" );
USER user;
bool bTesting = false;

HWND hwndSpellText, hwndCodeText, hwndPWText, hwndCmdGenerate, hwndName, hwndDate;
BOOL CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BITSET encode(BITSET b);
BITSET decode(BITSET b);
void assemble(const PDATA pd, PBITSET pbs);
void disassemble(PDATA pd, const PBITSET pbs);
ULONG StrToNum(const TCHAR *udata, int udatalen, int base);
TCHAR* NumToStr(TCHAR *RetData, long number, int base);
string hash(const PBITSET pbs);
BITSET unhash(const string s);
string pad(TCHAR *ch, const int numDigits=4);
string padi(const ULONG ul, const int numDigits=4);
string spell(const string s);
bool fileExist(const char *filename);


int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, char* cmdParam, int cmdShow) {
    TheInstance = hInst;
    string s = cmdParam;
	stringstream ss(stringstream::in | stringstream::out);


	string suser;	
	std::list<string>  ls;
	std::vector<string> vParams;
	stringtok (ls, s);
	for (list<string>::const_iterator i = ls.begin(); i != ls.end(); ++i) {
		vParams.push_back(*i);
	}
	for (int i=0; i<vParams.size(); i++) {
        std::string s = vParams[i];
        
        if(s=="-d") {
		// decode hash
			BITSET tbs;				
			PDATA pd = new DATA;	
			tbs = unhash((vParams[i+1]).c_str());
			tbs = decode(tbs);
			disassemble(pd, &tbs);
			
			if (fileExist(db.dbName().c_str())) {
				Query q(db);
				q.get_result("SELECT id, login, firstname, lastname, department, priv FROM tUsers WHERE id="+ padi(pd->userid,0)  +"");
				if (q.fetch_row()) {
					// registerd user
					user.id = q.getval();
					user.login = q.getstr();
					user.firstname = q.getstr();
					user.lastname = q.getstr();
					user.department = q.getstr();
					user.priv = q.getval();
					s = user.firstname + " " +  user.lastname + " (" + user.login + ") - " + user.department;
				} else {			
					s = padi(pd->userid,0);
				}	
				q.free_result();	
			}
		
			ss << padi(pd->day,2) << "/" << padi(pd->month,2) << "/" << padi(pd->year,2) << " " << padi(pd->hour,2) 
					<< ":" << padi(pd->minute,2) << " UTC | user:" << s << " | ref:" 
					<< pd->ref;
					
			delete pd;			
			
			MessageBox(0, ss.str().c_str(), "Decoded hash", 0);
			exit(0);
        }
        
        if (s=="-c") {
		// check if hash is in db
		
		}
        
        if (s=="-test") {
			bTesting = TRUE;
		}
        
        if(s=="-createdb") {
            //search for user
            for(int j=0; j<vParams.size()-1; j++) {
                if (vParams[j] == "-u"){
                    suser=vParams[j+1];
                    continue;
                }
            }
            if (suser=="") {
                MessageBox(0, "Error: Username is required when rebuilding tables. Terminating...","ERROR", 0);
                exit(1);
            }
            Query q(db);
            q.execute("DROP TABLE IF EXISTS \"main\".\"tUsers\";");
            q.execute("DROP TABLE IF EXISTS \"main\".\"tKeys\";");

            q.execute("CREATE TABLE \"main\".\"tUsers\" (\"id\" INTEGER PRIMARY KEY, \"login\" TEXT, \"firstname\" TEXT, \"lastname\" TEXT, \"department\" TEXT, \"priv\" INTEGER);");
            q.execute("CREATE TABLE \"main\".\"tKeys\" (\"id\" INTEGER PRIMARY KEY, \"hash\" TEXT, \"date\" DATE, \"userid\" INTEGER, \"ref\" INTEGER);");
            q.execute("INSERT INTO tUsers (id, login, priv) VALUES (null, \""+suser+"\", 2);");
        }
	
    }
   	// check if db exist
	if (!fileExist(db.dbName().c_str())) {
		MessageBox(0, "Database not found or connection error!\r\n\r\nCannot continue execution. Terminating...", "ERROR",0);
		exit(1);
	}      
	return DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_MAIN),HWND_DESKTOP,DialogProc,0L);
}

BOOL CALLBACK DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) {
	TCHAR buffer[255];
	string s;
	stringstream ss(stringstream::in | stringstream::out);
	SYSTEMTIME st;
	PDATA pd;
	DWORD nUserName;
	char acUserName[100];
	
	switch(uMsg) {
		case WM_INITDIALOG:
			hwndSpellText = GetDlgItem(hwndDlg, IDC_SPELL);
			hwndPWText = GetDlgItem(hwndDlg, IDC_PW);
			hwndCodeText = GetDlgItem(hwndDlg, IDC_CODE);
			hwndCmdGenerate = GetDlgItem(hwndDlg, IDC_GENERATE);
			hwndName = GetDlgItem(hwndDlg, IDC_NAME);
			hwndDate = GetDlgItem(hwndDlg, IDC_DATE);
			
			nUserName = sizeof(acUserName);			
			
			if (GetUserName(acUserName, &nUserName)!=0) {
				// check if user is authorised to issue code and store user id, 0 if not authorised
				Query q(db);
				
				q.get_result("SELECT id, login, firstname, lastname, department, priv FROM tUsers WHERE login=\""+string(acUserName)+"\"");
				if (q.fetch_row()) {
					// registerd user
					user.id = q.getval();
					user.login = q.getstr();
					user.firstname = q.getstr();
					user.lastname = q.getstr();
					user.department = q.getstr();
					user.priv = q.getval();
				} else {
					// guest login
					user.id = 0;
					user.login = acUserName;
					user.firstname = "";
					user.lastname = "";
					user.department = "";
					user.priv = 0;
				}
				 q.free_result();
				
				if (user.priv == 0) {
					EnableWindow(hwndCmdGenerate, false);
					EnableWindow(hwndPWText, false);
					SendMessage(hwndCodeText, WM_SETTEXT, 0 , (LPARAM)TEXT("Authorisation failed."));
					SendMessage(hwndSpellText, WM_SETTEXT, 0 , (LPARAM)TEXT("You are not permitted to issue authorisation codes.\r\n\r\nIf you think you should be please contact NetOps HQ NMC Managers."));
				}
				if (user.id != 0) {
					s = user.firstname + " " +  user.lastname + " (" + user.department + ")";
				} else {
					s = user.login;
				}
				
				SendMessage(hwndName, WM_SETTEXT, 0 , (LPARAM)TEXT(s.c_str()));
				
				char buffer[100];
				time_t curr=time(0);// current local time
				tm gmt=*gmtime(&curr);// convert curr to GMT, store as tm
				sprintf(buffer, "%s/%s/%s %s:%s", padi(gmt.tm_mday,2).c_str(), padi(gmt.tm_mon,2).c_str(),
						padi(gmt.tm_year-100, 2).c_str(), padi(gmt.tm_hour,2).c_str(), padi(gmt.tm_min,2).c_str());
				SendMessage(hwndDate, WM_SETTEXT, 0 , (LPARAM)buffer);
				
			}
			return TRUE;
		case WM_CLOSE:
			EndDialog(hwndDlg,wParam);
			return TRUE;
		case WM_SIZE:
			//MoveWindow(hwndDlg, winRect.left, winRect.top, winRect.right, winRect.bottom, TRUE);
			return 0; // event handled
		case WM_COMMAND:
			if(wParam==IDC_OK)
				EndDialog(hwndDlg,wParam);
			if (wParam==IDC_GENERATE) {
				GetWindowText(hwndPWText, buffer, 20);
				//EnableWindow(hwndCmdGenerate, false);
				//EnableWindow(hwndPWText, false);
				
				BITSET tbs(0);
				ostringstream sss, ss;
				pd = new DATA;
				pd->userid = user.id;
				pd->ref = atol(buffer);//123456;				
				assemble(pd, &tbs);
				
				
				tbs = encode(tbs);
				ss << hash(&tbs); // ss contains converted to base 30 strings
								
				SendMessage(hwndCodeText, WM_SETTEXT, 0 , (LPARAM)ss.str().c_str());
				SendMessage(hwndSpellText, WM_SETTEXT, 0 , (LPARAM)(spell(ss.str())).c_str());
				EnableWindow(hwndCmdGenerate, false);
				EnableWindow(hwndPWText, false);			
				
				//store hash
				if (user.priv >0) {
					Query q(db);
					
					std::ostringstream sSQL;
					
					sSQL << "INSERT INTO tKeys (id, hash, date, userid, ref) VALUES (null, \"" << ss.str() <<
						"\", \"" << pd->dateStr << "\", " << user.id << ", " << pd->ref << ");";
					if (!bTesting) {
						q.execute(sSQL.str());
					}
				}
				
				delete pd;
			}
			return TRUE;
	}
	return FALSE;
}



BITSET encode(BITSET b) {
	BITSET out(0);
	for (int i=0; i<64; i++) {
		out[CONV[i]]= b[i];
	}
	return (out);
}

BITSET decode(BITSET b) {
	BITSET out(0);
	for (int i=0; i<64; i++) {
		out[i] = b[CONV[i]];
	}
	return (out);	
}

void assemble(const PDATA pd, PBITSET pbs) {
	// constructs bitset from data
	
	
	// fill in date/time struct
	time_t curr=time(0);// current local time
	tm gmt=*gmtime(&curr);// convert curr to GMT, store as tm
	//time_t utc=(mktime(&local));// convert GMT tm to GMT time_t
	
	pd->minute = gmt.tm_min;
	pd->hour = gmt.tm_hour;
	pd->day = gmt.tm_mday;
	pd->month = gmt.tm_mon;
	pd->year = gmt.tm_year-100; //+1900 - 2000
	
	char buffer[100];	
	sprintf(buffer, "%s/%s/%s %s:%s", padi(gmt.tm_mday,2).c_str(), padi(gmt.tm_mon,2).c_str(),
						padi(gmt.tm_year-100, 2).c_str(), padi(gmt.tm_hour,2).c_str(), padi(gmt.tm_min,2).c_str());
	pd->dateStr = string(buffer);
	
	pbs->reset();
	
	BITSET bs(0);
	
	// start assembling
	// day|month|year|hour|min|user|ref
	//  5 |  4  |  6 |  5 | 6 | 10 | 28 

	//bs <<= 5;	
	bs |= (pd->day & 0x1F);
	
	bs <<= 4;
	bs |= (pd->month & 0xF);
	
	bs <<= 6;
	bs |= (pd->year & 0x3F);

	bs <<= 5;
	bs |= (pd->hour & 0x1F);
	
	bs <<= 6;
	bs |= (pd->minute & 0x3F);

	bs <<= 10;
	bs |= (pd->userid & 0x3FF);

	bs <<= 28;
	bs |= (pd->ref & 0x0FFFFFFF);

	(*pbs) = bs;
}

void disassemble(PDATA pd, const PBITSET pbs) {
	// fills in data structure from bitset
	BITSET cpybs(*pbs);
	BITSET mask(0x0FFFFFFF);
	
	cpybs &= mask;
	pd->ref = cpybs.to_ulong();
	
	cpybs = *pbs;
	cpybs >>= 28;
	cpybs &= 0x3FF;
	pd->userid = cpybs.to_ulong();
	
	cpybs = *pbs;
	cpybs >>= (28+10);
	cpybs  &= 0x3F;
	pd->minute = cpybs.to_ulong();
	
	cpybs = *pbs;
	cpybs >>= (28+10+6);
	cpybs  &= 0x1F;
	pd->hour = cpybs.to_ulong();
	
	cpybs = *pbs;
	cpybs >>= (28+10+6+5);
	cpybs  &= 0x3F;
	pd->year = cpybs.to_ulong(); 
	
	cpybs = *pbs;
	cpybs >>= (28+10+6+5+6);
	cpybs  &= 0xF;
	pd->month = cpybs.to_ulong(); 
	
	cpybs = *pbs;
	cpybs >>= (28+10+6+5+6+4);
	cpybs  &= 0x3F;
	pd->day = cpybs.to_ulong(); 
		
}


/*				
					wsprintf(buffer, "%d", CONV[i]);
					s += buffer;
					wsprintf(buffer, "%d", CONV[i+1]);
					s += " --> ";
					s += buffer;
					s += "\r\n";
*/					


ULONG StrToNum(const TCHAR *udata, int udatalen, int base)
{
	long index;
	const TCHAR numdigits[] = TEXT("0123456789ABCDEFHJKLMNPRTUWXYZ");
	ULONG digitValue = 0;
	ULONG RetVal = 0;
	TCHAR digits[sizeof(numdigits)+1];
	TCHAR *dataVal;
	TCHAR data[512] ;
	//copy the data to our variable
	_tcscpy(data, udata);
	//convert it to upper case
	_tcsupr(data);
	ZeroMemory(digits, sizeof(digits));
	//copy the number of digits supported by base in digits
	_tcsncpy(digits, numdigits, base);
	for(index = 0; index < udatalen; index++)
	{
		//is the number there
		dataVal = _tcschr(digits, data[index] );
		if(dataVal != 0 )
		{
			//if it is subtract where to start point
			digitValue = long(dataVal - digits);
			//increment Retval with digitvalue
			RetVal = RetVal * base + digitValue;
		}
	}
	//return the result
	return RetVal;
}

TCHAR* NumToStr(TCHAR *RetData, ULONG number, int base)
{
	long index = 0;
	const TCHAR numdigits[] = TEXT("0123456789ABCDEFHJKLMNPRTUWXYZ");
	ULONG digitValue = 0;
	TCHAR digits[sizeof(numdigits) + 1];
	TCHAR RetVal[512];
	TCHAR CurVal = 0;	
	ZeroMemory(RetVal, sizeof(RetVal));
	// only base supported are from 2 to 36
	if(base < 2 || base > 30 ) return NULL;
	ZeroMemory(digits, sizeof(digits));
	_tcsncpy(digits, numdigits, base);
	while(number)
	{
		digitValue = number % base;
		number = number / base;
		RetVal[index++] = digits[digitValue];
	}
	//since string we have got is in reversed format
	//eg 100 will be 001 so we have to reverse it
	//and put the value in our variable
	ZeroMemory(RetData, _tcslen(RetVal)+1);
	int i = 0;
	for(index = _tcslen(RetVal) - 1; index > -1; index--)
	{
		//start reversing
		RetData[i++] = RetVal[index];
	}
	//return the result
	return RetData;
}

string hash(const PBITSET pbs) {
	ostringstream s;
	TCHAR Data[256];
	
	BITSET cpybs(*pbs);
	BITSET mask(0x0000FFFF); //16bit
	
	ZeroMemory(Data, sizeof(Data));
	cpybs &= mask;
	NumToStr(Data, cpybs.to_ulong(),30);  
	s << pad(Data) <<"-";
	
	ZeroMemory(Data, sizeof(Data));
	cpybs = *pbs;
	cpybs >>= 16;
	cpybs &= mask;
	NumToStr(Data, cpybs.to_ulong(),30); 
	s << pad(Data) <<"-";

	ZeroMemory(Data, sizeof(Data));
	cpybs = *pbs;
	cpybs >>= (16+16);
	cpybs &= mask;
	NumToStr(Data, cpybs.to_ulong(),30); 
	s << pad(Data) <<"-";
	
		ZeroMemory(Data, sizeof(Data));
	cpybs = *pbs;
	cpybs >>= (16+16+16);
	cpybs &= mask;
	NumToStr(Data, cpybs.to_ulong(),30); 
	s << pad(Data) ;
	
	return (string)(s.str());
	
}

BITSET unhash(const string s) {
	std::list<string>  ls;
	ostringstream ss;
	std::vector<ULONG> v;
	
	//00UK-0003-1TKZ-0KAP	
	stringtok (ls, s, "-");
	
	for (list<string>::const_iterator i = ls.begin(); i != ls.end(); ++i) {
		v.push_back(StrToNum( ((*i).c_str()), 4, 30));
	} 	
	
	BITSET bs = BITSET(0);
	bs |= v[3];
	
	bs <<= 16;
	bs |= (v[2] & 0x0000FFFF);	
	
	bs <<= 16;
	bs |= (v[1] & 0x0000FFFF);

	bs <<= 16;
	bs |= (v[0] & 0x0000FFFF);	
	
	return bs;
}

string pad(TCHAR *ch, const int numDigits) {
	string s = ch;
	for (int i=s.length(); i<numDigits; i++) {
		s = "0"+s;
	}
	return s;
}

string padi(const ULONG ul, const int numDigits) {
	
	TCHAR buffer[20];
	sprintf(buffer, "%u", ul);
	string s = pad(buffer, numDigits);
	return s;
}

string spell(const string str) {
	char c;
	string s;
	
	for (int i=0; i<str.size(); i++) {
		c = str[i];
		if (c>='0' && c<='Z') {
			s += PHWORDS[c-'0'];
			s += "\r\n";
		} else if (c=='-') {
			s += "\r\n";
		}
		
	}
	return s;
}

bool fileExist( const char *filename ) {
  struct stat buffer;
  if ( stat( filename, &buffer ) ) return FALSE ;
  return TRUE ;
}
