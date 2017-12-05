
#include "Tokenizer.h"

#include <iostream>

using namespace std;

//Testing the class
int _main()
{
    //Test CIsSpace() predicate
	{
      cout << "Test CIsSpace() predicate:" << endl;
	  //The Results Vector
      vector<string> oResult;
	  //Call Tokeniker
	  CTokenizer<>::Tokenize(oResult, " wqd \t hgwh \t sdhw \r\n kwqo \r\n  dk ");
	  //Display Results
      for(int i=0; i<oResult.size(); i++)
        cout << oResult[i] << endl;
	}

	//Test CIsComma() predicate
	{
      cout << "Test CIsComma() predicate:" << endl;
	  //The Results Vector
      vector<string> oResult;
      //Call Tokeniker
      CTokenizer<CIsComma>::Tokenize(oResult, "wqd,hgwh,sdhw,kwqo,dk", CIsComma());
      //Display Results
      for(int i=0; i<oResult.size(); i++)
        cout << oResult[i] << endl;
    }

	//Test CIsFromString predicate
	{
      cout << "Test CIsFromString() predicate:" << endl;
      //The Results Vector
      vector<string> oResult;
      //Call Tokeniker
      CTokenizer<CIsFromString>::Tokenize(oResult, ":wqd,;hgwh,:,sdhw,:;kwqo;dk,", CIsFromString(",;:"));
      //Display Results
	  cout << "Display strings:" << endl;
      for(int i=0; i<oResult.size(); i++)
        cout << oResult[i] << endl;
	}

    return 0;
}

