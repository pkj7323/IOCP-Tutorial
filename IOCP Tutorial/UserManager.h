#pragma once
#include "ErrorCode.h"

class User;

class UserManager
{
	INT32 m_maxUseCnt = 0;
	INT32 m_currentUserCnt = 0;

	std::vector<User*> m_userObjPool;
	std::unordered_map<std::string, int> m_userIdMap;
public:
	UserManager() = default;
	~UserManager() = default;
	void Init(const INT32 maxUserCnt_);

	INT32 GetCurrentUserCnt() const { return m_currentUserCnt; }
	INT32 GetMaxUserCnt() const { return m_maxUseCnt; }
	void IncreaseUserCnt() { ++m_currentUserCnt; }

	void DecreaseUserCnt()
	{
		if (m_currentUserCnt > 0)
		{
			--m_currentUserCnt;
		}
	}

	ERROR_CODE AddUser(char* userID_, int clientIndex_);
	INT32 FindUserIndexByID(char* userID_);
	void DeleteUserInfo(User* user_);

	User* GetUserByConnIdx(int clientIndex_)
	{
		return m_userObjPool[clientIndex_];
	}
	


};
