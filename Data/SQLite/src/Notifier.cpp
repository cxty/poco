//
// Notifier.cpp
//
// $Id: //poco/Main/Data/SQLite/src/Notifier.cpp#5 $
//
// Library: SQLite
// Package: SQLite
// Module:  Notifier
//
// Implementation of Notifier
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/Data/SQLite/Notifier.h"


namespace Poco {
namespace Data {
namespace SQLite {


Notifier::Notifier(const Session& session, EnabledEventType enabled):
	_session(session)
{
	if (enabled & SQLITE_NOTIFY_UPDATE)   enableUpdate();
	if (enabled & SQLITE_NOTIFY_COMMIT)   enableCommit();
	if (enabled & SQLITE_NOTIFY_ROLLBACK) enableRollback();
}


Notifier::Notifier(const Session& session, const Any& value, EnabledEventType enabled):
	_session(session),
	_value(value)
{
	if (enabled & SQLITE_NOTIFY_UPDATE)   enableUpdate();
	if (enabled & SQLITE_NOTIFY_COMMIT)   enableCommit();
	if (enabled & SQLITE_NOTIFY_ROLLBACK) enableRollback();
}


Notifier::~Notifier()
{
	disableAll();
}


bool Notifier::enableUpdate()
{
	Poco::Mutex::ScopedLock l(_mutex);
	
	if (Utility::registerUpdateHandler(Utility::dbHandle(_session), &sqliteUpdateCallbackFn, this))
		_enabledEvents |= SQLITE_NOTIFY_UPDATE;
	
	return updateEnabled();
}


bool Notifier::disableUpdate()
{
	Poco::Mutex::ScopedLock l(_mutex);
	
	if (Utility::registerUpdateHandler(Utility::dbHandle(_session), (Utility::UpdateCallbackType) 0, this))
		_enabledEvents &= ~SQLITE_NOTIFY_UPDATE;
	
	return !updateEnabled();
}


bool Notifier::updateEnabled() const
{
	return 0 != (_enabledEvents & SQLITE_NOTIFY_UPDATE);
}


bool Notifier::enableCommit()
{
	Poco::Mutex::ScopedLock l(_mutex);
	
	if (Utility::registerUpdateHandler(Utility::dbHandle(_session), &sqliteCommitCallbackFn, this))
		_enabledEvents |= SQLITE_NOTIFY_COMMIT;
	
	return commitEnabled();
}


bool Notifier::disableCommit()
{
	Poco::Mutex::ScopedLock l(_mutex);
	
	if (Utility::registerUpdateHandler(Utility::dbHandle(_session), (Utility::CommitCallbackType) 0, this))
		_enabledEvents &= ~SQLITE_NOTIFY_COMMIT;
	
	return !commitEnabled();
}


bool Notifier::commitEnabled() const
{
	return 0 != (_enabledEvents & SQLITE_NOTIFY_COMMIT);
}


bool Notifier::enableRollback()
{
	Poco::Mutex::ScopedLock l(_mutex);
	
	if (Utility::registerUpdateHandler(Utility::dbHandle(_session), &sqliteRollbackCallbackFn, this))
		_enabledEvents |= SQLITE_NOTIFY_ROLLBACK;
	
	return rollbackEnabled();
}


bool Notifier::disableRollback()
{
	Poco::Mutex::ScopedLock l(_mutex);

	if (Utility::registerUpdateHandler(Utility::dbHandle(_session), (Utility::RollbackCallbackType) 0, this))
		_enabledEvents &= ~SQLITE_NOTIFY_ROLLBACK;
	
	return !rollbackEnabled();
}


bool Notifier::rollbackEnabled() const
{
	return 0 != (_enabledEvents & SQLITE_NOTIFY_ROLLBACK);
}


bool Notifier::enableAll()
{
	return enableUpdate() && enableCommit() && enableRollback();
}


bool Notifier::disableAll()
{
	return disableUpdate() && disableCommit() && disableRollback();
}


void Notifier::sqliteUpdateCallbackFn(void* pVal, int opCode, const char* pDB, const char* pTable, Poco::Int64 row)
{
	poco_check_ptr(pVal);
	Notifier* pV = reinterpret_cast<Notifier*>(pVal);
	if (opCode == Utility::OPERATION_INSERT)
	{
		pV->_row = row;
		pV->insert.notify(pV);
	}
	else if (opCode == Utility::OPERATION_UPDATE)
	{
		pV->_row = row;
		pV->update.notify(pV);
	}
	else if (opCode == Utility::OPERATION_DELETE)
	{
		pV->_row = row;
		pV->erase.notify(pV);
	}
}


int Notifier::sqliteCommitCallbackFn(void* pVal)
{
	Notifier* pV = reinterpret_cast<Notifier*>(pVal);

	try
	{
		pV->commit.notify(pV);
	}
	catch (...)
	{
		return -1;
	}

	return 0;
}


void Notifier::sqliteRollbackCallbackFn(void* pVal)
{
	Notifier* pV = reinterpret_cast<Notifier*>(pVal);
	pV->rollback.notify(pV);
}


} } } // namespace Poco::Data::SQLite
