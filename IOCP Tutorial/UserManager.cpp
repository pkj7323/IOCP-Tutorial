#include "pch.h"
#include "User.h"
#include "UserManager.h"

void UserManager::Init(const INT32 maxUserCnt_)
{
	m_maxUseCnt = maxUserCnt_;
	m_userObjPool = std::vector<User*>(m_maxUseCnt);

	for (auto i = 0; i < m_maxUseCnt; i++)
	{
		m_userObjPool[i] = new User();
		m_userObjPool[i]->Init(i);
	}
}

ERROR_CODE UserManager::AddUser(char* userID_, int clientIndex_)
{
	auto userIndex = clientIndex_;

	m_userObjPool[userIndex]->SetLogin(userID_);
	m_userIdMap.insert(std::pair<char*, int>(userID_, clientIndex_));

	return ERROR_CODE::NONE;
}

INT32 UserManager::FindUserIndexByID(char* userID_)
{
	if (auto res = m_userIdMap.find(userID_); res != m_userIdMap.end())
	{
		return res->second;
	}
	
	return -1;
}

void UserManager::DeleteUserInfo(User* user_)
{
	m_userIdMap.erase(user_->GetUserID());
	user_->Clear();
}


