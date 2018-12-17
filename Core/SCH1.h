#ifndef __DP_LIB_FTP_SYNC_SCH1_
#define __DP_LIB_FTP_SYNC_SCH1_

#include <Types/Random.h>

using __DP_LIB_NAMESPACE__::Random;
using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::Int;
using __DP_LIB_NAMESPACE__::UInt;

namespace ENC {
	class SCH1 {
		private:
			Random _rand;
			String _key;
		public:
			SCH1(const String & key) :_key(key) {
				_rand = Random(key);
			}
			void Reset(Int step = 0) {
				_rand = Random(_key);
				for (UInt i = 0; i < step; i++) {
					_rand.RandInt();
				}
			}
			Int GetValue(Int max = -1) {
				if (max == -1)
					return _rand.RandInt();
				return _rand.RandInt(max);
			}


	};

	String DecryptName(const String & filename, SCH1 & crypt);
	String EncryptName(const String & filename, SCH1 & crypt);
	bool CopyFile(const String & from, const String & to, SCH1 & crypt);
	String EncryptString(__DP_LIB_NAMESPACE__::Ostream & out, const String & txt, SCH1 & crypt);
	String DecryptString(const String & txt, SCH1 & crypt);
}

#endif
