#include "SCH1.h"
#include <Converter/Converter.h>

using __DP_LIB_NAMESPACE__::ByteToHex;
using __DP_LIB_NAMESPACE__::HexToInt;
using __DP_LIB_NAMESPACE__::UChar;
using __DP_LIB_NAMESPACE__::Ifstream;
using __DP_LIB_NAMESPACE__::Ofstream;
using __DP_LIB_NAMESPACE__::Char;

namespace ENC{
	String DecryptName(const String & filename, SCH1 & crypt){
		crypt.Reset();
		String res = "";
		const String& EncName = filename;
		UChar prev = 0;
		for (UInt i = 0; i < EncName.size() ; i ++ ){
			if (EncName[i] == '/' || EncName[i] == '\\') {
				res += EncName[i];
				crypt.Reset();
				continue;
			}
			UChar c = EncName[i];
			if (prev != 0) {
				Int val = HexToInt(prev) * 16 + HexToInt(c);
				Char s = (Char) (val ^ crypt.GetValue(256));
				res += s;
				prev = 0;
			} else {
				prev = c;
				continue;
			}
		}
		return res;
	}

	String EncryptName(const String & filename, SCH1 & crypt) {
		crypt.Reset();
		String res = "";
		const String & DecName = filename;
		for (UInt i = 0; i < DecName.size(); i++ ){
			if (DecName[i] == '/' || DecName[i] == '\\') {
				res += DecName[i];
				crypt.Reset();
				continue;
			}
			UChar c = DecName[i];
			c = c ^ crypt.GetValue(256);
			res += ByteToHex(c / 16);
			res += ByteToHex(c%16);
		}
		return res;
	}

	String EncryptString(__DP_LIB_NAMESPACE__::Ostream & out, const String & txt, SCH1 & crypt) {
		crypt.Reset();
		String res = "";
		for (int i = 0; i < txt.size(); i++) {
			Char k = txt[i];
			Char b = (Char) (k ^ crypt.GetValue(256));
			res += b;
			out.put(b);
		}
		return res;
	}
	String DecryptString(const String & txt, SCH1 & crypt) {
		crypt.Reset();
		String res = "";
		for (int i = 0; i < txt.size(); i++) {
			Char k = txt[i];
			Char b = (Char) (k ^ crypt.GetValue(256));
			res += b;
		}
		return res;
	}

	bool CopyFile(const String & from, const String & to, SCH1 & crypt) {
		Ifstream in;
		in.open(from, std::ios::binary);
		if (in.fail())
			throw EXCEPTION("Files " + from + " is not found");

		Ofstream out;
		out.open(to, std::ios::binary);
		if (out.fail())
			throw EXCEPTION("Files " + from + " is not found");

		__DP_LIB_NAMESPACE__::Char c;
		while (in.get(c)) {
			Char k = (Char) ( c ^ crypt.GetValue(256));
			out.put(k);
		}
		out.close();
		in.close();
		return true;
	}
}
