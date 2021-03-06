#include <string>
using namespace std;
// The std:: prefix is not used here, for readability, and a line like
// "using namespace std;" is dangerous to have in a header file.

template <typename Container>
void
stringtok (Container &container, string const &in,
           const char * const delimiters = " \t\n:")
{
    const string::size_type len = in.length();
          string::size_type i = 0;

    while ( i < len )
    {
        // eat leading whitespace
        i = in.find_first_not_of (delimiters, i);
        if (i == string::npos)
            return;   // nothing left but white space

        // find the end of the token
        string::size_type j = in.find_first_of (delimiters, i);

        // push token
        if (j == string::npos) {
            container.push_back (in.substr(i));
            return;
        } else
            container.push_back (in.substr(i, j-i));

        // set up for next loop
        i = j + 1;
    }
}

/* tricks

{
    vector<string> tokens;

    string str("Split me up! Word1 Word2 Word3.");

    Tokenize(str, tokens);

    copy(tokens.begin(), tokens.end(), ostream_iterator<string>(cout, ", "));
}

*/

