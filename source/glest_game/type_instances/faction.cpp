// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "faction.h"

#include <algorithm>
#include <cassert>

#include "resource_type.h"
#include "unit.h"
#include "util.h"
#include "sound_renderer.h"
#include "renderer.h"
#include "tech_tree.h"
#include "game.h"
#include "config.h"
#include "randomgen.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using Shared::Util::RandomGen;

namespace Glest { namespace Game {

bool CommandGroupUnitSorterId::operator()(const int l, const int r) {
	const Unit *lUnit = faction->findUnit(l);
	const Unit *rUnit = faction->findUnit(r);

	if(!lUnit) {
		printf("Error lUnit == NULL for id = %d factionIndex = %d\n",l,faction->getIndex());

		for(unsigned int i = 0; i < (unsigned int)faction->getUnitCount(); ++i) {
			printf("%u / %d id = %d [%s]\n",i,faction->getUnitCount(),faction->getUnit(i)->getId(),faction->getUnit(i)->getType()->getName(false).c_str());
		}
	}
	if(!rUnit) {
		printf("Error rUnit == NULL for id = %d factionIndex = %d\n",r,faction->getIndex());

		for(unsigned int i = 0; i < (unsigned int)faction->getUnitCount(); ++i) {
			printf("%u / %d id = %d [%s]\n",i,faction->getUnitCount(),faction->getUnit(i)->getId(),faction->getUnit(i)->getType()->getName(false).c_str());
		}
	}

	CommandGroupUnitSorter sorter;
	return sorter.compare(lUnit, rUnit);
}

bool CommandGroupUnitSorter::operator()(const Unit *l, const Unit *r) {
	return compare(l, r);
}

bool CommandGroupUnitSorter::compare(const Unit *l, const Unit *r) {
	//printf("l [%p] r [%p] <>",l,r);

	if(!l) {
		printf("Error l == NULL\n");
	}
	if(!r) {
		printf("Error r == NULL\n");
	}

	assert(l && r);

	if(l == NULL || r == NULL)
		printf("Unit l [%s - %d] r [%s - %d]\n",
			(l != NULL ? l->getType()->getName(false).c_str() : "null"),
			(l != NULL ? l->getId() : -1),
			(r != NULL ? r->getType()->getName(false).c_str() : "null"),
			(r != NULL ? r->getId() : -1));


	bool result = false;
	// If comparer is null or dead
	if(r == NULL || r->isAlive() == false) {
		// if source is null or dead also
		if((l == NULL || l->isAlive() == false)) {
			return false;
		}
		return true;
	}
	else if((l == NULL || l->isAlive() == false)) {
		return false;
	}

//	const Command *command= l->getCurrrentCommandThreadSafe();
//	const Command *commandPeer = r->getCurrrentCommandThreadSafe();
	const Command *command= l->getCurrCommand();
	const Command *commandPeer = r->getCurrCommand();

	//Command *command= this->unit->getCurrCommand();

	// Are we moving or attacking
	if( command != NULL && command->getCommandType() != NULL &&
			(command->getCommandType()->getClass() == ccMove ||
			 command->getCommandType()->getClass() == ccAttack)  &&
		command->getUnitCommandGroupId() > 0) {
		int curCommandGroupId = command->getUnitCommandGroupId();

		//Command *commandPeer = j.unit->getCurrrentCommandThreadSafe();
		//Command *commandPeer = j.unit->getCurrCommand();

		// is comparer a valid command
		if(commandPeer == NULL || commandPeer->getCommandType() == NULL) {
			result = true;
		}
		// is comparer command the same type?
		else if(commandPeer->getCommandType()->getClass() !=
				command->getCommandType()->getClass()) {
			result = true;
		}
		// is comparer command groupid invalid?
		else if(commandPeer->getUnitCommandGroupId() < 0) {
			result = true;
		}
		// If comparer command group id is less than current group id
		else if(curCommandGroupId != commandPeer->getUnitCommandGroupId()) {
			result = curCommandGroupId < commandPeer->getUnitCommandGroupId();
		}
		else {
			float unitDist = l->getCenteredPos().dist(command->getPos());
			float unitDistPeer = r->getCenteredPos().dist(commandPeer->getPos());

			// Closest unit in commandgroup
			result = (unitDist < unitDistPeer);
		}
	}
	else if(command == NULL && commandPeer != NULL) {
		result = false;
	}
//	else if(command == NULL && j.unit->getCurrrentCommandThreadSafe() == NULL) {
//		return this->unit->getId() < j.unit->getId();
//	}
	else {
		//Command *commandPeer = j.unit->getCurrrentCommandThreadSafe();
		//if( commandPeer != NULL && commandPeer->getCommandType() != NULL &&
		//	(commandPeer->getCommandType()->getClass() != ccMove &&
		//	 commandPeer->getCommandType()->getClass() != ccAttack)) {
			result = (l->getId() < r->getId());
		//}
		//else {
		//	result = (l->getId() < r->getId());
		//}
	}

	//printf("Sorting, unit [%d - %s] cmd [%s] | unit2 [%d - %s] cmd [%s] result = %d\n",this->unit->getId(),this->unit->getFullName().c_str(),(this->unit->getCurrCommand() == NULL ? "NULL" : this->unit->getCurrCommand()->toString().c_str()),j.unit->getId(),j.unit->getFullName().c_str(),(j.unit->getCurrCommand() == NULL ? "NULL" : j.unit->getCurrCommand()->toString().c_str()),result);

	return result;
}

void Faction::sortUnitsByCommandGroups() {
	MutexSafeWrapper safeMutex(unitsMutex,string(__FILE__) + "_" + intToStr(__LINE__));
	//printf("====== sortUnitsByCommandGroups for faction # %d [%s] unitCount = %d\n",this->getIndex(),this->getType()->getName().c_str(),units.size());
	//for(unsigned int i = 0; i < units.size(); ++i) {
	//	printf("%d / %d [%p] <>",i,units.size(),&units[i]);
//	//	printf("i = %d [%p]\n",i,&units[i]);
//		if(Unit::isUnitDeleted(units[i]) == true) {
//			printf("i = %d [%p]\n",i,&units[i]);
//			throw megaglest_runtime_error("unit already deleted!");
//		}
	//}
	//printf("\nSorting\n");

	//std::sort(units.begin(),units.end(),CommandGroupUnitSorter());

	//printf("====== Done sorting for faction # %d [%s] unitCount = %d\n",this->getIndex(),this->getType()->getName().c_str(),units.size());

	//unsigned int originalUnitSize = (unsigned int)units.size();

	std::vector<int> unitIds;
	for(unsigned int i = 0; i < units.size(); ++i) {
		int unitId = units[i]->getId();
		if(this->findUnit(unitId) == NULL) {
			printf("#1 Error unitId not found for id = %d [%s] factionIndex = %d\n",unitId,units[i]->getType()->getName(false).c_str(),this->getIndex());

			for(unsigned int j = 0; j < units.size(); ++j) {
				printf("%u / %d id = %d [%s]\n",j,(int)units.size(),units[j]->getId(),units[j]->getType()->getName(false).c_str());
			}
		}
		unitIds.push_back(unitId);
	}
	CommandGroupUnitSorterId sorter;
	sorter.faction = this;
	std::stable_sort(unitIds.begin(),unitIds.end(),sorter);

	units.clear();
	for(unsigned int i = 0; i < unitIds.size(); ++i) {

		int unitId = unitIds[i];
		if(this->findUnit(unitId) == NULL) {
			printf("#2 Error unitId not found for id = %d factionIndex = %d\n",unitId,this->getIndex());

			for(unsigned int j = 0; j < units.size(); ++j) {
				printf("%u / %d id = %d [%s]\n",j,(int)units.size(),units[j]->getId(),units[j]->getType()->getName(false).c_str());
			}
		}

		units.push_back(this->findUnit(unitId));
	}

	//assert(originalUnitSize == units.size());
}

// =====================================================
//	class FactionThread
// =====================================================

FactionThread::FactionThread(Faction *faction) : BaseThread() {
	this->triggerIdMutex = new Mutex(CODE_AT_LINE);
	this->faction = faction;
	this->masterController = NULL;
	uniqueID = "FactionThread";
}

FactionThread::~FactionThread() {
	this->faction = NULL;
	this->masterController = NULL;
	delete this->triggerIdMutex;
	this->triggerIdMutex = NULL;
}

void FactionThread::setQuitStatus(bool value) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d value = %d\n",__FILE__,__FUNCTION__,__LINE__,value);

	BaseThread::setQuitStatus(value);
	if(value == true) {
		signalPathfinder(-1);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}

void FactionThread::signalPathfinder(int frameIndex) {
	if(frameIndex >= 0) {
		static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
		MutexSafeWrapper safeMutex(triggerIdMutex,mutexOwnerId);
		this->frameIndex.first = frameIndex;
		this->frameIndex.second = false;

		safeMutex.ReleaseLock();
	}
	semTaskSignalled.signal();
}

void FactionThread::setTaskCompleted(int frameIndex) {
	if(frameIndex >= 0) {
		static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
		MutexSafeWrapper safeMutex(triggerIdMutex,mutexOwnerId);
		if(this->frameIndex.first == frameIndex) {
			this->frameIndex.second = true;
		}
		safeMutex.ReleaseLock();
	}
}

bool FactionThread::canShutdown(bool deleteSelfIfShutdownDelayed) {
	bool ret = (getExecutingTask() == false);
	if(ret == false && deleteSelfIfShutdownDelayed == true) {
	    setDeleteSelfOnExecutionDone(deleteSelfIfShutdownDelayed);
	    deleteSelfIfRequired();
	    signalQuit();
	}

	return ret;
}

bool FactionThread::isSignalPathfinderCompleted(int frameIndex) {
	if(getRunningStatus() == false) {
		return true;
	}
	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
	MutexSafeWrapper safeMutex(triggerIdMutex,mutexOwnerId);
	//bool result = (event != NULL ? event->eventCompleted : true);
	bool result = (this->frameIndex.first == frameIndex && this->frameIndex.second == true);

	//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] worker thread this = %p, this->frameIndex.first = %d, this->frameIndex.second = %d\n",__FILE__,__FUNCTION__,__LINE__,this,this->frameIndex.first,this->frameIndex.second);

	safeMutex.ReleaseLock();
	return result;
}

void FactionThread::execute() {
	string codeLocation = "1";
    RunningStatusSafeWrapper runningStatus(this);
	try {
		//setRunningStatus(true);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] ****************** STARTING worker thread this = %p\n",__FILE__,__FUNCTION__,__LINE__,this);

		bool minorDebugPerformance = false;
		Chrono chrono;

		codeLocation = "2";
		//unsigned int idx = 0;
		for(;this->faction != NULL;) {
			if(getQuitStatus() == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}

			semTaskSignalled.waitTillSignalled();

			codeLocation = "3";
			//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			static string masterSlaveOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
			MasterSlaveThreadControllerSafeWrapper safeMasterController(masterController,20000,masterSlaveOwnerId);
			//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(getQuitStatus() == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}

			static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
            MutexSafeWrapper safeMutex(triggerIdMutex,mutexOwnerId);
            bool executeTask = (this->frameIndex.first >= 0);
			int currentTriggeredFrameIndex = this->frameIndex.first;

            //if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] frameIndex = %d this = %p executeTask = %d\n",__FILE__,__FUNCTION__,__LINE__,frameIndex.first, this, executeTask);

            safeMutex.ReleaseLock();

			codeLocation = "5";
            //printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

            if(executeTask == true) {
				codeLocation = "6";
				ExecutingTaskSafeWrapper safeExecutingTaskMutex(this);

				if(this->faction == NULL) {
					throw megaglest_runtime_error("this->faction == NULL");
				}
				World *world = this->faction->getWorld();
				if(world == NULL) {
					throw megaglest_runtime_error("world == NULL");
				}

				codeLocation = "7";
				//Config &config= Config::getInstance();
				//bool sortedUnitsAllowed = config.getBool("AllowGroupedUnitCommands","true");
				bool sortedUnitsAllowed = false;
				if(sortedUnitsAllowed == true) {
					this->faction->sortUnitsByCommandGroups();
				}

				codeLocation = "8";
				static string mutexOwnerId2 = string(__FILE__) + string("_") + intToStr(__LINE__);
				MutexSafeWrapper safeMutex(faction->getUnitMutex(),mutexOwnerId2);

				//if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();
				if(minorDebugPerformance) chrono.start();

				//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				codeLocation = "9";
				int unitCount = this->faction->getUnitCount();
				for(int j = 0; j < unitCount; ++j) {
					codeLocation = "10";
					Unit *unit = this->faction->getUnit(j);
					if(unit == NULL) {
						throw megaglest_runtime_error("unit == NULL");
					}

					codeLocation = "11";
					int64 elapsed1 = 0;
					if(minorDebugPerformance) elapsed1 = chrono.getMillis();

					bool update = unit->needToUpdate();

					codeLocation = "12";
					if(minorDebugPerformance && (chrono.getMillis() - elapsed1) >= 1) printf("Faction [%d - %s] #1-unit threaded updates on frame: %d for [%d] unit # %d, unitCount = %d, took [%lld] msecs\n",faction->getStartLocationIndex(),faction->getType()->getName(false).c_str(),currentTriggeredFrameIndex,faction->getUnitPathfindingListCount(),j,unitCount,(long long int)chrono.getMillis() - elapsed1);

					//update = true;
					if(update == true)
					{
						codeLocation = "13";
						if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
							int64 updateProgressValue = unit->getUpdateProgress();
							int64 speed = unit->getCurrSkill()->getTotalSpeed(unit->getTotalUpgrade());
							int64 df = unit->getDiagonalFactor();
							int64 hf = unit->getHeightFactor();
							bool changedActiveCommand = unit->isChangedActiveCommand();

							char szBuf[8096]="";
							snprintf(szBuf,8096,"unit->needToUpdate() returned: %d updateProgressValue: %lld speed: %lld changedActiveCommand: %d df: %lld hf: %lld",update,(long long int)updateProgressValue,(long long int)speed,changedActiveCommand,(long long int)df,(long long int)hf);
							unit->logSynchDataThreaded(__FILE__,__LINE__,szBuf);
						}

						int64 elapsed2 = 0;
						if(minorDebugPerformance) elapsed2 = chrono.getMillis();

						if(world->getUnitUpdater() == NULL) {
							throw megaglest_runtime_error("world->getUnitUpdater() == NULL");
						}

						world->getUnitUpdater()->updateUnitCommand(unit,currentTriggeredFrameIndex);

						codeLocation = "15";
						if(minorDebugPerformance && (chrono.getMillis() - elapsed2) >= 1) printf("Faction [%d - %s] #2-unit threaded updates on frame: %d for [%d] unit # %d, unitCount = %d, took [%lld] msecs\n",faction->getStartLocationIndex(),faction->getType()->getName(false).c_str(),currentTriggeredFrameIndex,faction->getUnitPathfindingListCount(),j,unitCount,(long long int)chrono.getMillis() - elapsed2);
					}
					else {
						codeLocation = "16";
						if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
							int64 updateProgressValue = unit->getUpdateProgress();
							int64 speed = unit->getCurrSkill()->getTotalSpeed(unit->getTotalUpgrade());
							int64 df = unit->getDiagonalFactor();
							int64 hf = unit->getHeightFactor();
							bool changedActiveCommand = unit->isChangedActiveCommand();

							char szBuf[8096]="";
							snprintf(szBuf,8096,"unit->needToUpdate() returned: %d updateProgressValue: %lld speed: %lld changedActiveCommand: %d df: %lld hf: %lld",update,(long long int)updateProgressValue,(long long int)speed,changedActiveCommand,(long long int)df,(long long int)hf);
							unit->logSynchDataThreaded(__FILE__,__LINE__,szBuf);
						}
					}
				}

				codeLocation = "17";
				if(minorDebugPerformance && chrono.getMillis() >= 1) printf("Faction [%d - %s] threaded updates on frame: %d for [%d] units took [%lld] msecs\n",faction->getStartLocationIndex(),faction->getType()->getName(false).c_str(),currentTriggeredFrameIndex,faction->getUnitPathfindingListCount(),(long long int)chrono.getMillis());

				//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				safeMutex.ReleaseLock();

				codeLocation = "18";
				//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				setTaskCompleted(currentTriggeredFrameIndex);

				//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
            }

            //printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			codeLocation = "19";
			if(getQuitStatus() == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] ****************** ENDING worker thread this = %p\n",__FILE__,__FUNCTION__,__LINE__,this);
	}
	catch(const exception &ex) {
		//setRunningStatus(false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Loc [%s] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str(),ex.what());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		throw megaglest_runtime_error(ex.what());
	}
	catch(...) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] UNKNOWN error Loc [%s]\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
}


// =====================================================
// 	class Faction
// =====================================================

Faction::Faction() {
	init();
}

void Faction::init() {
	unitsMutex = new Mutex(CODE_AT_LINE);
	texture = NULL;
	//lastResourceTargettListPurge = 0;
	cachingDisabled=false;
	factionDisconnectHandled=false;
	workerThread = NULL;

	world=NULL;
	scriptManager=NULL;
	factionType=NULL;
	index=0;
	teamIndex=0;
	startLocationIndex=0;
	thisFaction=false;
	currentSwitchTeamVoteFactionIndex = -1;
	allowSharedTeamUnits = false;

	loadWorldNode = NULL;
	techTree = NULL;

	control = ctClosed;

	overridePersonalityType = fpt_EndCount;

	upgradeManager = UpgradeManager();
}

Faction::~Faction() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//Renderer &renderer= Renderer::getInstance();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//renderer.endTexture(rsGame,texture);
	//texture->end();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(workerThread != NULL) {
		workerThread->signalQuit();
		if(workerThread->shutdownAndWait() == true) {
			delete workerThread;
		}
		workerThread = NULL;
	}

	MutexSafeWrapper safeMutex(unitsMutex,string(__FILE__) + "_" + intToStr(__LINE__));
	deleteValues(units.begin(), units.end());
	units.clear();

	safeMutex.ReleaseLock();

	//delete texture;
	texture = NULL;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	delete unitsMutex;
	unitsMutex = NULL;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Faction::end() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(workerThread != NULL) {
		workerThread->signalQuit();
		if(workerThread->shutdownAndWait() == true) {
			delete workerThread;
		}
		workerThread = NULL;
	}

	MutexSafeWrapper safeMutex(unitsMutex,string(__FILE__) + "_" + intToStr(__LINE__));
	deleteValues(units.begin(), units.end());
	units.clear();

	safeMutex.ReleaseLock();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Faction::notifyUnitAliveStatusChange(const Unit *unit) {
	if(unit != NULL) {
		if(unit->isAlive() == true) {
			aliveUnitListCache[unit->getId()] = unit;

			if(unit->getType()->isMobile() == true) {
				mobileUnitListCache[unit->getId()] = unit;
			}
		}
		else {
			aliveUnitListCache.erase(unit->getId());
			mobileUnitListCache.erase(unit->getId());
			beingBuiltUnitListCache.erase(unit->getId());
		}
	}
}

void Faction::notifyUnitTypeChange(const Unit *unit, const UnitType *newType) {
	if(unit != NULL) {
		if(unit->getType()->isMobile() == true) {
			mobileUnitListCache.erase(unit->getId());
		}

		if(newType != NULL && newType->isMobile() == true) {
			mobileUnitListCache[unit->getId()] = unit;
		}
	}
}

void Faction::notifyUnitSkillTypeChange(const Unit *unit, const SkillType *newType) {
	if(unit != NULL) {
		if(unit->isBeingBuilt() == true) {
			beingBuiltUnitListCache.erase(unit->getId());
		}
		if(newType != NULL && newType->getClass() == scBeBuilt) {
			beingBuiltUnitListCache[unit->getId()] = unit;
		}
	}
}

bool Faction::hasAliveUnits(bool filterMobileUnits, bool filterBuiltUnits) const {
	bool result = false;
	if(aliveUnitListCache.empty() == false) {
		if(filterMobileUnits == true) {
			result = (mobileUnitListCache.empty() == false);
		}
		else {
			result = true;
		}

		if(result == true && filterBuiltUnits == true) {
			result = (beingBuiltUnitListCache.empty() == true);
		}
	}
	return result;
}

FactionPersonalityType Faction::getPersonalityType() const {
	if(overridePersonalityType != fpt_EndCount) {
		return overridePersonalityType;
	}
	return factionType->getPersonalityType();
}

int Faction::getAIBehaviorStaticOverideValue(AIBehaviorStaticValueCategory type) const {
	return factionType->getAIBehaviorStaticOverideValue(type);
}

void Faction::addUnitToMovingList(int unitId) {
	unitsMovingList[unitId] = getWorld()->getFrameCount();
}
void Faction::removeUnitFromMovingList(int unitId) {
	unitsMovingList.erase(unitId);
}

int Faction::getUnitMovingListCount() {
	return (int)unitsMovingList.size();
}

void Faction::addUnitToPathfindingList(int unitId) {
	//printf("ADD (1) Faction [%d - %s] threaded updates for [%d] units\n",this->getStartLocationIndex(),this->getType()->getName().c_str(),unitsPathfindingList.size());
	unitsPathfindingList[unitId] = getWorld()->getFrameCount();
	//printf("ADD (2) Faction [%d - %s] threaded updates for [%d] units\n",this->getStartLocationIndex(),this->getType()->getName().c_str(),unitsPathfindingList.size());
}
void Faction::removeUnitFromPathfindingList(int unitId) {
	unitsPathfindingList.erase(unitId);
}

int Faction::getUnitPathfindingListCount() {
	//printf("GET Faction [%d - %s] threaded updates for [%d] units\n",this->getStartLocationIndex(),this->getType()->getName().c_str(),unitsPathfindingList.size());
	return (int)unitsPathfindingList.size();
}

void Faction::clearUnitsPathfinding() {
	//printf("CLEAR Faction [%d - %s] threaded updates for [%d] units\n",this->getStartLocationIndex(),this->getType()->getName().c_str(),unitsPathfindingList.size());
	if(unitsPathfindingList.empty() == false) {
		unitsPathfindingList.clear();
	}
}

bool Faction::canUnitsPathfind() {
	bool result = true;
	if(control == ctCpuEasy  || control == ctCpu ||
	   control == ctCpuUltra || control == ctCpuMega) {
		//printf("AI player for faction index: %d (%s) current pathfinding: %d\n",index,factionType->getName().c_str(),getUnitPathfindingListCount());

		const int MAX_UNITS_PATHFINDING_PER_FRAME = 10;
		result = (getUnitPathfindingListCount() <= MAX_UNITS_PATHFINDING_PER_FRAME);
		if(result == false) {
			//printf("WARNING limited AI player for faction index: %d (%s) current pathfinding: %d\n",index,factionType->getName().c_str(),getUnitPathfindingListCount());
		}
	}
	return result;
}

void Faction::setLockedUnitForFaction(const UnitType *ut, bool lock) {
	if (lock) {
		lockedUnits.insert(ut);
	} else {
		std::set<const UnitType*>::iterator it;
		it=lockedUnits.find(ut);
		if(it!=lockedUnits.end()) {
			lockedUnits.erase(it);
		}
	}

}

void Faction::signalWorkerThread(int frameIndex) {
	if(workerThread != NULL) {
		workerThread->signalPathfinder(frameIndex);
	}
}

bool Faction::isWorkerThreadSignalCompleted(int frameIndex) {
	if(workerThread != NULL) {
		return workerThread->isSignalPathfinderCompleted(frameIndex);
	}
	return true;
}


void Faction::init(
	FactionType *factionType, ControlType control, TechTree *techTree, Game *game,
	int factionIndex, int teamIndex, int startLocationIndex, bool thisFaction, bool giveResources,
	const XmlNode *loadWorldNode)
{
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	this->techTree = techTree;
	this->loadWorldNode = loadWorldNode;
	this->control= control;
	this->factionType= factionType;
	this->startLocationIndex= startLocationIndex;
	this->index= factionIndex;
	this->teamIndex= teamIndex;
	this->thisFaction= thisFaction;
	this->world= game->getWorld();
	this->scriptManager= game->getScriptManager();
	//cachingDisabled = (Config::getInstance().getBool("DisableCaching","false") == true);
	cachingDisabled = false;

	resources.resize(techTree->getResourceTypeCount());
	store.resize(techTree->getResourceTypeCount());

	if(loadWorldNode == NULL) {
		for(int index = 0; index < techTree->getResourceTypeCount(); ++index) {
			const ResourceType *rt	= techTree->getResourceType(index);
			int resourceAmount		= giveResources ? factionType->getStartingResourceAmount(rt): 0;
			resources[index].init(rt, resourceAmount);
			store[index].init(rt, 0);

			this->world->initTeamResource(rt,this->teamIndex,0);
		}
	}
	//initialize cache
	for(int index = 0; index < techTree->getResourceTypeCount(); ++index) {
		const ResourceType *rt	= techTree->getResourceType(index);
		this->updateUnitTypeWithResourceCostCache(rt);
	}

	texture= Renderer::getInstance().newTexture2D(rsGame);
	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	if(texture) {
		string playerTexture = getGameCustomCoreDataPath(data_path, "data/core/faction_textures/faction" + intToStr(startLocationIndex) + ".tga");
		texture->load(playerTexture);
	}

	if(loadWorldNode != NULL) {
		loadGame(loadWorldNode, this->index,game->getGameSettings(),game->getWorld());
	}

	if( game->getGameSettings()->getPathFinderType() == pfBasic) {
		if(workerThread != NULL) {
			workerThread->signalQuit();
			if(workerThread->shutdownAndWait() == true) {
				delete workerThread;
			}
			workerThread = NULL;
		}
		static string mutexOwnerId = string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") + intToStr(__LINE__);
		this->workerThread = new FactionThread(this);
		this->workerThread->setUniqueID(mutexOwnerId);
		this->workerThread->start();
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ================== get ==================

bool Faction::hasUnitTypeWithResourceCostInCache(const ResourceType *rt) const {
	std::string resourceTypeName = rt->getName(false);
	std::map<std::string, bool>::const_iterator iterFind = resourceTypeCostCache.find(resourceTypeName);
	if(iterFind != resourceTypeCostCache.end()) {
		return iterFind->second;
	}
	return false;
}
void Faction::updateUnitTypeWithResourceCostCache(const ResourceType *rt) {
	std::string resourceTypeName = rt->getName(false);

	if(resourceTypeCostCache.find(resourceTypeName) == resourceTypeCostCache.end()) {
		resourceTypeCostCache[resourceTypeName] = hasUnitTypeWithResouceCost(rt);
	}
}

bool Faction::hasUnitTypeWithResouceCost(const ResourceType *rt) {
	for(int factionUnitTypeIndex = 0;
			factionUnitTypeIndex < getType()->getUnitTypeCount();
				++factionUnitTypeIndex) {

		const UnitType *ut = getType()->getUnitType(factionUnitTypeIndex);
		if(ut->getCost(rt) != NULL) {
			return true;
		}
	}
	return false;
}

const Resource *Faction::getResource(const ResourceType *rt,bool localFactionOnly) const {

	if(localFactionOnly == false &&
			world != NULL &&
				world->getGame() != NULL) {

		Game *game = world->getGame();
		if(game->isFlagType1BitEnabled(ft1_allow_shared_team_resources) == true) {
			return world->getResourceForTeam(rt, this->getTeam());
		}
	}

	for(int index = 0; index < (int)resources.size(); ++index) {
		if(rt == resources[index].getType()) {
			return &resources[index];
		}
	}

	printf("ERROR cannot find resource type [%s] in list:\n",(rt != NULL ? rt->getName().c_str() : "null"));
	for(int i=0; i < (int)resources.size(); ++i){
		printf("Index %d [%s]",i,resources[i].getType()->getName().c_str());
	}

	assert(false);
	return NULL;
}

int Faction::getStoreAmount(const ResourceType *rt,bool localFactionOnly) const {

	if(localFactionOnly == false &&
			world != NULL &&
				world->getGame() != NULL) {

		Game *game = world->getGame();
		if(game->isFlagType1BitEnabled(ft1_allow_shared_team_resources) == true) {
			return world->getStoreAmountForTeam(rt, this->getTeam());
		}
	}

	for(int index =0 ; index < (int)store.size(); ++index) {
		if(rt == store[index].getType()) {
			return store[index].getAmount();
		}
	}
	printf("ERROR cannot find store type [%s] in list:\n",(rt != NULL ? rt->getName().c_str() : "null"));
	for(int i=0; i < (int)store.size(); ++i){
		printf("Index %d [%s]",i,store[i].getType()->getName().c_str());
	}

	assert(false);
	return 0;
}

bool Faction::getCpuControl(bool enableServerControlledAI,bool isNetworkGame, NetworkRole role) const {
	bool result = false;
	if(enableServerControlledAI == false || isNetworkGame == false) {
			result = (control == ctCpuEasy ||control == ctCpu || control == ctCpuUltra || control == ctCpuMega);
	}
	else {
		if(isNetworkGame == true) {
			if(role == nrServer) {
				result = (control == ctCpuEasy ||control == ctCpu || control == ctCpuUltra || control == ctCpuMega);
			}
			else {
				result = (control == ctNetworkCpuEasy ||control == ctNetworkCpu || control == ctNetworkCpuUltra || control == ctNetworkCpuMega);
			}
		}
	}

	return result;
}

bool Faction::getCpuControl() const {
	return 	control == ctCpuEasy 		||control == ctCpu 			|| control == ctCpuUltra 		|| control == ctCpuMega ||
			control == ctNetworkCpuEasy ||control == ctNetworkCpu 	|| control == ctNetworkCpuUltra || control == ctNetworkCpuMega;
}

// ==================== upgrade manager ====================

void Faction::startUpgrade(const UpgradeType *ut){
	upgradeManager.startUpgrade(ut, index);
}

void Faction::cancelUpgrade(const UpgradeType *ut){
	upgradeManager.cancelUpgrade(ut);
}

void Faction::finishUpgrade(const UpgradeType *ut){
	upgradeManager.finishUpgrade(ut);
	if(world->getThisFaction()!=NULL && this->getIndex()==world->getThisFaction()->getIndex()){
		Console *console=world->getGame()->getConsole();
		console->addStdMessage("UpgradeFinished",": " + formatString(ut->getName(true)));
	}
	for(int i=0; i<getUnitCount(); ++i){
		getUnit(i)->applyUpgrade(ut);
	}
}

// ==================== reqs ====================

//checks if all required units and upgrades are present and maxUnitCount is within limit
bool Faction::reqsOk(const RequirableType *rt) const {
	assert(rt != NULL);
	//required units
    for(int i = 0; i < rt->getUnitReqCount(); ++i) {
        bool found = false;
        for(int j = 0; j < getUnitCount(); ++j) {
			Unit *unit= getUnit(j);
            const UnitType *ut= unit->getType();
            if(rt->getUnitReq(i) == ut && unit->isOperative()) {
                found= true;
                break;
            }
        }
		if(found == false) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);
            return false;
		}
    }

	//required upgrades
    for(int i = 0; i < rt->getUpgradeReqCount(); ++i) {
		if(upgradeManager.isUpgraded(rt->getUpgradeReq(i)) == false) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);
			return false;
		}
    }

    if(dynamic_cast<const UnitType *>(rt) != NULL ) {
    	const UnitType *producedUnitType= dynamic_cast<const UnitType *>(rt);
   		if(producedUnitType != NULL && producedUnitType->getMaxUnitCount() > 0) {
			if(producedUnitType->getMaxUnitCount() <= getCountForMaxUnitCount(producedUnitType)) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__, __LINE__);
		        return false;
			}
   		}

   		if(producedUnitType != NULL && isUnitLocked(producedUnitType)) {
   			return false;
   		}
    }

	return true;
}

int Faction::getCountForMaxUnitCount(const UnitType *unitType) const{
	int count=0;
	//calculate current unit count
   	for(int j=0; j<getUnitCount(); ++j){
		Unit *unit= getUnit(j);
        const UnitType *currentUt= unit->getType();
        if(unitType==currentUt && unit->isOperative()){
            count++;
        }
        //check if there is any command active which already produces this unit
        count=count+unit->getCountOfProducedUnits(unitType);
    }
	return count;
}


bool Faction::reqsOk(const CommandType *ct) const {
	assert(ct != NULL);
	if(ct == NULL) {
	    throw megaglest_runtime_error("In [Faction::reqsOk] ct == NULL");
	}

	if(ct->getProduced() != NULL && reqsOk(ct->getProduced()) == false) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] reqsOk FAILED\n",__FILE__,__FUNCTION__,__LINE__);
		return false;
	}

	if(ct->getClass() == ccUpgrade) {
		const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
		if(upgradeManager.isUpgradingOrUpgraded(uct->getProducedUpgrade())) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] upgrade check FAILED\n",__FILE__,__FUNCTION__,__LINE__);
			return false;
		}
	}

	return reqsOk(static_cast<const RequirableType*>(ct));
}

// ================== cost application ==================

//apply costs except static production (start building/production)
bool Faction::applyCosts(const ProducibleType *p,const CommandType *ct) {
	bool ignoreResourceCosts = false;
	if(ct != NULL && ct->getClass() == ccMorph) {
		const MorphCommandType *mct = dynamic_cast<const MorphCommandType *>(ct);
		if(mct != NULL) {
			ignoreResourceCosts = mct->getIgnoreResourceRequirements();
		}
	}

	if(ignoreResourceCosts == false) {
		if(checkCosts(p,ct) == false) {
			return false;
		}

		assert(p != NULL);
		//for each unit cost spend it
		//pass 2, decrease resources, except negative static costs (ie: farms)
		for(int i=0; i<p->getCostCount(); ++i) {
			const Resource *r= p->getCost(i);
			if(r == NULL) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"cannot apply costs for p [%s] %d of %d costs resource is null",p->getName(false).c_str(),i,p->getCostCount());
				throw megaglest_runtime_error(szBuf);
			}

			const ResourceType *rt= r->getType();
			if(rt == NULL) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"cannot apply costs for p [%s] %d of %d costs resourcetype [%s] is null",p->getName(false).c_str(),i,p->getCostCount(),r->getDescription(false).c_str());
				throw megaglest_runtime_error(szBuf);
			}
			int cost= r->getAmount();
			if((cost > 0 || (rt->getClass() != rcStatic)) && rt->getClass() != rcConsumable) {
				incResourceAmount(rt, -(cost));
			}

		}
	}
    return true;
}

//apply discount (when a morph ends)
void Faction::applyDiscount(const ProducibleType *p, int discount)
{
	assert(p != NULL);
	//increase resources
	for(int i=0; i<p->getCostCount(); ++i)
	{
		const ResourceType *rt= p->getCost(i)->getType();
		assert(rt != NULL);
        int cost= p->getCost(i)->getAmount();
		if((cost > 0 || (rt->getClass() != rcStatic)) && rt->getClass() != rcConsumable)
		{
            incResourceAmount(rt, cost*discount/100);
		}
    }
}

//apply static production (for starting units)
void Faction::applyStaticCosts(const ProducibleType *p,const CommandType *ct) {
	assert(p != NULL);
	bool ignoreResourceCosts = false;
	if(ct != NULL && ct->getClass() == ccMorph) {
		const MorphCommandType *mct = dynamic_cast<const MorphCommandType *>(ct);
		if(mct != NULL) {
			ignoreResourceCosts = mct->getIgnoreResourceRequirements();
		}
	}

	if(ignoreResourceCosts == false) {
		//decrease static resources
		for(int i=0; i < p->getCostCount(); ++i) {
			const ResourceType *rt= p->getCost(i)->getType();
			//assert(rt != NULL);
			if(rt == NULL) {
				throw megaglest_runtime_error(string(__FUNCTION__) + " rt == NULL for ProducibleType [" + p->getName(false) + "] index: " + intToStr(i));
			}
			if(rt->getClass() == rcStatic) {
				int cost= p->getCost(i)->getAmount();
				if(cost > 0) {
					incResourceAmount(rt, -cost);
				}
			}
		}
	}
}

//apply static production (when a mana source is done)
void Faction::applyStaticProduction(const ProducibleType *p,const CommandType *ct) {
	assert(p != NULL);

	bool ignoreResourceCosts = false;
	if(ct != NULL && ct->getClass() == ccMorph) {
		const MorphCommandType *mct = dynamic_cast<const MorphCommandType *>(ct);
		if(mct != NULL) {
			ignoreResourceCosts = mct->getIgnoreResourceRequirements();
		}
	}

	if(ignoreResourceCosts == false) {
		//decrease static resources
		for(int i=0; i<p->getCostCount(); ++i) {
			const ResourceType *rt= p->getCost(i)->getType();
			assert(rt != NULL);
			if(rt->getClass() == rcStatic) {
				int cost= p->getCost(i)->getAmount();
				if(cost < 0) {
					incResourceAmount(rt, -cost);
				}
			}
		}
	}
}

//deapply all costs except static production (usually when a building is cancelled)
void Faction::deApplyCosts(const ProducibleType *p,const CommandType *ct) {
	assert(p != NULL);

	bool ignoreResourceCosts = false;
	if(ct != NULL && ct->getClass() == ccMorph) {
		const MorphCommandType *mct = dynamic_cast<const MorphCommandType *>(ct);
		if(mct != NULL) {
			ignoreResourceCosts = mct->getIgnoreResourceRequirements();
		}
	}

	if(ignoreResourceCosts == false) {
		//increase resources
		for(int i=0; i<p->getCostCount(); ++i) {
			const ResourceType *rt= p->getCost(i)->getType();
			assert(rt != NULL);
			int cost= p->getCost(i)->getAmount();
			if((cost > 0 || (rt->getClass() != rcStatic)) && rt->getClass() != rcConsumable) {
				incResourceAmount(rt, cost);
			}
		}
	}
}

//deapply static costs (usually when a unit dies)
void Faction::deApplyStaticCosts(const ProducibleType *p,const CommandType *ct) {
	assert(p != NULL);

	bool ignoreResourceCosts = false;
	if(ct != NULL && ct->getClass() == ccMorph) {
		const MorphCommandType *mct = dynamic_cast<const MorphCommandType *>(ct);
		if(mct != NULL) {
			ignoreResourceCosts = mct->getIgnoreResourceRequirements();
		}
	}

	if(ignoreResourceCosts == false) {
		//decrease resources
		for(int i=0; i<p->getCostCount(); ++i) {
			const ResourceType *rt= p->getCost(i)->getType();
			assert(rt != NULL);
			if(rt->getClass() == rcStatic) {
				if(rt->getRecoup_cost() == true) {
					int cost= p->getCost(i)->getAmount();
					incResourceAmount(rt, cost);
				}
			}
		}
	}
}

//deapply static costs, but not negative costs, for when building gets killed
void Faction::deApplyStaticConsumption(const ProducibleType *p,const CommandType *ct) {
	assert(p != NULL);

	bool ignoreResourceCosts = false;
	if(ct != NULL && ct->getClass() == ccMorph) {
		const MorphCommandType *mct = dynamic_cast<const MorphCommandType *>(ct);
		if(mct != NULL) {
			ignoreResourceCosts = mct->getIgnoreResourceRequirements();
		}
	}

	if(ignoreResourceCosts == false) {
		//decrease resources
		for(int i=0; i<p->getCostCount(); ++i) {
			const ResourceType *rt= p->getCost(i)->getType();
			assert(rt != NULL);
			if(rt->getClass() == rcStatic) {
				int cost= p->getCost(i)->getAmount();
				if(cost > 0) {
					incResourceAmount(rt, cost);
				}
			}
		}
	}
}

//apply resource on interval (cosumable resouces)
void Faction::applyCostsOnInterval(const ResourceType *rtApply) {

	// For each Resource type we store in the int a total consumed value, then
	// a vector of units that consume the resource type
	std::map<const ResourceType *, std::pair<int, std::vector<Unit *> > > resourceIntervalUsage;

	// count up consumables usage for the interval
	for(int j = 0; j < getUnitCount(); ++j) {
		Unit *unit = getUnit(j);
		if(unit->isOperative() == true) {
			for(int k = 0; k < unit->getType()->getCostCount(); ++k) {
				const Resource *resource = unit->getType()->getCost(k);
				if(resource->getType() == rtApply && resource->getType()->getClass() == rcConsumable && resource->getAmount() != 0) {
					if(resourceIntervalUsage.find(resource->getType()) == resourceIntervalUsage.end()) {
						resourceIntervalUsage[resource->getType()] = make_pair<int, std::vector<Unit *> >(0,std::vector<Unit *>());
					}
					// Negative cost means accumulate the resource type
					resourceIntervalUsage[resource->getType()].first += -resource->getAmount();

					// If the cost > 0 then the unit is a consumer
					if(resource->getAmount() > 0) {
						resourceIntervalUsage[resource->getType()].second.push_back(unit);
					}
				}
			}
		}
	}

	// Apply consumable resource usage
	if(resourceIntervalUsage.empty() == false) {
		for(std::map<const ResourceType *, std::pair<int, std::vector<Unit *> > >::iterator iter = resourceIntervalUsage.begin();
																							iter != resourceIntervalUsage.end();
																							++iter) {
			// Apply resource type usage to faction resource store
			const ResourceType *rt = iter->first;
			int resourceTypeUsage = iter->second.first;
			incResourceAmount(rt, resourceTypeUsage);

			// Check if we have any unit consumers
			if(getResource(rt)->getAmount() < 0) {
				resetResourceAmount(rt);

				// Apply consequences to consumer units of this resource type
				std::vector<Unit *> &resourceConsumers = iter->second.second;

				for(int i = 0; i < (int)resourceConsumers.size(); ++i) {
					Unit *unit = resourceConsumers[i];

					//decrease unit hp
					if(scriptManager->getPlayerModifiers(this->index)->getConsumeEnabled() == true) {
						bool decHpResult = unit->decHp(unit->getType()->getTotalMaxHp(unit->getTotalUpgrade()) / 3);
						if(decHpResult) {
							unit->setCauseOfDeath(ucodStarvedResource);
							world->getStats()->die(unit->getFactionIndex(),unit->getType()->getCountUnitDeathInStats());
							scriptManager->onUnitDied(unit);
						}
						StaticSound *sound= static_cast<const DieSkillType *>(unit->getType()->getFirstStOfClass(scDie))->getSound();
						if(sound != NULL &&
							(thisFaction == true || world->showWorldForPlayer(world->getThisTeamIndex()) == true)) {
							SoundRenderer::getInstance().playFx(sound);
						}
					}
				}
			}
		}
	}
}

bool Faction::checkCosts(const ProducibleType *pt,const CommandType *ct) {
	assert(pt != NULL);

	bool ignoreResourceCosts = false;
	if(ct != NULL && ct->getClass() == ccMorph) {
		const MorphCommandType *mct = dynamic_cast<const MorphCommandType *>(ct);
		if(mct != NULL) {
			ignoreResourceCosts = mct->getIgnoreResourceRequirements();
		}
		//printf("Checking costs = %d for commandtype:\n%s\n",ignoreResourceCosts,mct->getDesc(NULL).c_str());
	}

	if(ignoreResourceCosts == false) {
		//for each unit cost check if enough resources
		for(int i = 0; i < pt->getCostCount(); ++i) {
			const ResourceType *rt= pt->getCost(i)->getType();
			int cost= pt->getCost(i)->getAmount();
			if(cost > 0) {
				int available= getResource(rt)->getAmount();
				if(cost > available){
					return false;
				}
			}
		}
	}

	return true;
}

// ================== diplomacy ==================

bool Faction::isAlly(const Faction *faction) {
	assert(faction != NULL);
	return (teamIndex == faction->getTeam() ||
			faction->getTeam() == GameConstants::maxPlayers -1 + fpt_Observer);
}

// ================== misc ==================

void Faction::incResourceAmount(const ResourceType *rt, int amount) {
	if (world != NULL && world->getGame() != NULL
			&& world->getGame()->isFlagType1BitEnabled(
					ft1_allow_shared_team_resources) == true) {
		for(int i=0; i < (int)resources.size(); ++i) {
			Resource *r= &resources[i];
			if(r->getType()==rt) {
				r->setAmount(r->getAmount()+amount);
				if(r->getType()->getClass() != rcStatic && (getResource(rt,false)->getAmount()+amount)>getStoreAmount(rt,false)) {
					r->setAmount(getStoreAmount(rt,false)-(getResource(rt,false)->getAmount()-r->getAmount()));
				}
				return;
			}
		}
	} else {
		for(int i=0; i < (int)resources.size(); ++i) {
			Resource *r= &resources[i];
			if(r->getType()==rt) {
				r->setAmount(r->getAmount()+amount);
				if(r->getType()->getClass() != rcStatic && r->getAmount()>getStoreAmount(rt)) {
					r->setAmount(getStoreAmount(rt));
				}
				return;
			}
		}
	}
	assert(false);
}

void Faction::setResourceBalance(const ResourceType *rt, int balance){
	for(int i=0; i < (int)resources.size(); ++i){
		Resource *r= &resources[i];
		if(r->getType()==rt){
			r->setBalance(balance);
			return;
		}
	}
	assert(false);
}

Unit *Faction::findUnit(int id) const {
	UnitMap::const_iterator itFound = unitMap.find(id);
	if(itFound == unitMap.end()) {
		return NULL;
	}
	return itFound->second;
}

void Faction::addUnit(Unit *unit) {
	MutexSafeWrapper safeMutex(unitsMutex,string(__FILE__) + "_" + intToStr(__LINE__));
	units.push_back(unit);
	unitMap[unit->getId()] = unit;
}

void Faction::removeUnit(Unit *unit){
	MutexSafeWrapper safeMutex(unitsMutex,string(__FILE__) + "_" + intToStr(__LINE__));

	assert(units.size()==unitMap.size());

	int unitId = unit->getId();
	for(int i=0; i < (int)units.size(); ++i) {
		if(units[i]->getId() == unitId) {
			units.erase(units.begin()+i);
			unitMap.erase(unitId);
			assert(units.size() == unitMap.size());
			return;
		}
	}

	throw megaglest_runtime_error("Could not remove unit from faction!");
	//assert(false);
}

void Faction::addStore(const UnitType *unitType) {
	assert(unitType != NULL);
	for(int newUnitStoredResourceIndex = 0;
			newUnitStoredResourceIndex < unitType->getStoredResourceCount();
			++newUnitStoredResourceIndex) {
		const Resource *newUnitStoredResource = unitType->getStoredResource(newUnitStoredResourceIndex);

		for(int currentStoredResourceIndex = 0;
				currentStoredResourceIndex < (int)store.size();
				++currentStoredResourceIndex) {
			Resource *storedResource= &store[currentStoredResourceIndex];

			if(storedResource->getType() == newUnitStoredResource->getType()) {
				storedResource->setAmount(storedResource->getAmount() + newUnitStoredResource->getAmount());
			}
		}
	}
}

void Faction::removeStore(const UnitType *unitType){
	assert(unitType != NULL);
	for(int i=0; i<unitType->getStoredResourceCount(); ++i){
		const Resource *r= unitType->getStoredResource(i);
		for(int j=0; j < (int)store.size(); ++j){
			Resource *storedResource= &store[j];
			if(storedResource->getType() == r->getType()){
				storedResource->setAmount(storedResource->getAmount() - r->getAmount());
			}
		}
	}
	limitResourcesToStore();
}

void Faction::limitResourcesToStore() {
	if (world != NULL && world->getGame() != NULL
			&& world->getGame()->isFlagType1BitEnabled(
					ft1_allow_shared_team_resources) == true) {
		for(int i=0; i < (int)resources.size(); ++i) {
			Resource *r= &resources[i];
			const ResourceType *rt= r->getType();
			if(rt->getClass() != rcStatic && (getResource(rt,false)->getAmount())>getStoreAmount(rt,false)) {
				r->setAmount(getStoreAmount(rt,false)-(getResource(rt,false)->getAmount()-r->getAmount()));
			}
		}
	} else {
		for(int i=0; i < (int)resources.size(); ++i) {
			Resource *r= &resources[i];
			Resource *s= &store[i];
			if(r->getType()->getClass() != rcStatic && r->getAmount()>s->getAmount()) {
				r->setAmount(s->getAmount());
			}
		}
	}
}

void Faction::resetResourceAmount(const ResourceType *rt){
	for(int i=0; i < (int)resources.size(); ++i){
		if(resources[i].getType()==rt){
			resources[i].setAmount(0);
			return;
		}
	}
	assert(false);
}

bool Faction::isResourceTargetInCache(const Vec2i &pos, bool incrementUseCounter) {
	bool result = false;

	if(cachingDisabled == false) {
		if(cacheResourceTargetList.empty() == false) {
			std::map<Vec2i,int>::iterator iter = cacheResourceTargetList.find(pos);

			result = (iter != cacheResourceTargetList.end());
			if(result == true && incrementUseCounter == true) {
				iter->second++;
			}
		}
	}

	return result;
}

void Faction::addResourceTargetToCache(const Vec2i &pos,bool incrementUseCounter) {
	if(cachingDisabled == false) {

		bool duplicateEntry = isResourceTargetInCache(pos,incrementUseCounter);
		//bool duplicateEntry = false;

		if(duplicateEntry == false) {
			cacheResourceTargetList[pos] = 1;

			if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"[addResourceTargetToCache] pos [%s]cacheResourceTargetList.size() [" MG_SIZE_T_SPECIFIER "]",
								pos.getString().c_str(),cacheResourceTargetList.size());

				//unit->logSynchData(szBuf);
				SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"----------------------------------- START [%d] ------------------------------------------------\n",getFrameCount());
				SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"[%s::%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
				SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"%s\n",szBuf);
				SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"------------------------------------ END [%d] -------------------------------------------------\n",getFrameCount());
			}
		}
	}
}

void Faction::removeResourceTargetFromCache(const Vec2i &pos) {
	if(cachingDisabled == false) {
		if(cacheResourceTargetList.empty() == false) {
			std::map<Vec2i,int>::iterator iter = cacheResourceTargetList.find(pos);

			if(iter != cacheResourceTargetList.end()) {
				cacheResourceTargetList.erase(pos);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"[removeResourceTargetFromCache] pos [%s]cacheResourceTargetList.size() [" MG_SIZE_T_SPECIFIER "]",
									pos.getString().c_str(),cacheResourceTargetList.size());

					//unit->logSynchData(szBuf);
					SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"----------------------------------- START [%d] ------------------------------------------------\n",getFrameCount());
					SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"[%s::%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
					SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"%s\n",szBuf);
					SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"------------------------------------ END [%d] -------------------------------------------------\n",getFrameCount());
				}
			}
		}
	}
}

void Faction::addCloseResourceTargetToCache(const Vec2i &pos) {
	if(cachingDisabled == false) {
		if(cachedCloseResourceTargetLookupList.find(pos) == cachedCloseResourceTargetLookupList.end()) {
			const Map *map = world->getMap();
			const int harvestDistance = 5;

			for(int j = -harvestDistance; j <= harvestDistance; ++j) {
				for(int k = -harvestDistance; k <= harvestDistance; ++k) {
					Vec2i newPos = pos + Vec2i(j,k);
					if(isResourceTargetInCache(newPos) == false) {
						if(map->isInside(newPos.x, newPos.y)) {
							Resource *r = map->getSurfaceCell(map->toSurfCoords(newPos))->getResource();
							if(r != NULL) {
								addResourceTargetToCache(newPos);
								//cacheResourceTargetList[newPos] = 1;
							}
						}
					}
				}
			}

			cachedCloseResourceTargetLookupList[pos] = true;
		}
	}
}

Vec2i Faction::getClosestResourceTypeTargetFromCache(Unit *unit, const ResourceType *type, int frameIndex) {
	Vec2i result(-1);

	if(cachingDisabled == false) {
		if(cacheResourceTargetList.empty() == false) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"cacheResourceTargetList.size() [" MG_SIZE_T_SPECIFIER "]",cacheResourceTargetList.size());

				if(frameIndex < 0) {
					unit->logSynchData(__FILE__,__LINE__,szBuf);
				}
				else {
					unit->logSynchDataThreaded(__FILE__,__LINE__,szBuf);
				}
			}


			std::vector<Vec2i> deleteList;

			const int harvestDistance = 5;
			const Map *map = world->getMap();
			Vec2i pos = unit->getPos();

			bool foundCloseResource = false;
			// First look immediately around the unit's position

			// 0 means start looking leftbottom to top right
			int tryRadius = random.randRange(0,1);
			if(tryRadius == 0) {
				for(int j = -harvestDistance; j <= harvestDistance && foundCloseResource == false; ++j) {
					for(int k = -harvestDistance; k <= harvestDistance && foundCloseResource == false; ++k) {
						Vec2i newPos = pos + Vec2i(j,k);
						if(map->isInside(newPos) == true && isResourceTargetInCache(newPos) == false) {
							const SurfaceCell *sc = map->getSurfaceCell(map->toSurfCoords(newPos));
							if( sc != NULL && sc->getResource() != NULL) {
								const Resource *resource = sc->getResource();
								if(resource->getType() != NULL && resource->getType() == type) {
									if(result.x < 0 || unit->getPos().dist(newPos) < unit->getPos().dist(result)) {
										if(unit->isBadHarvestPos(newPos) == false) {
											result = newPos;
											foundCloseResource = true;
											break;
										}
									}
								}
							}
							else {
								deleteList.push_back(newPos);
							}
						}
					}
				}
			}
			// start looking topright to leftbottom
			else {
				for(int j = harvestDistance; j >= -harvestDistance && foundCloseResource == false; --j) {
					for(int k = harvestDistance; k >= -harvestDistance && foundCloseResource == false; --k) {
						Vec2i newPos = pos + Vec2i(j,k);
						if(map->isInside(newPos) == true && isResourceTargetInCache(newPos) == false) {
							const SurfaceCell *sc = map->getSurfaceCell(map->toSurfCoords(newPos));
							if( sc != NULL && sc->getResource() != NULL) {
								const Resource *resource = sc->getResource();
								if(resource->getType() != NULL && resource->getType() == type) {
									if(result.x < 0 || unit->getPos().dist(newPos) < unit->getPos().dist(result)) {
										if(unit->isBadHarvestPos(newPos) == false) {
											result = newPos;
											foundCloseResource = true;
											break;
										}
									}
								}
							}
							else {
								deleteList.push_back(newPos);
							}
						}
					}
				}
			}

			if(foundCloseResource == false) {
				// Now check the whole cache
				for(std::map<Vec2i,int>::iterator iter = cacheResourceTargetList.begin();
											  iter != cacheResourceTargetList.end() && foundCloseResource == false;
											  ++iter) {
					const Vec2i &cache = iter->first;
					if(map->isInside(cache) == true) {
						const SurfaceCell *sc = map->getSurfaceCell(map->toSurfCoords(cache));
						if( sc != NULL && sc->getResource() != NULL) {
							const Resource *resource = sc->getResource();
							if(resource->getType() != NULL && resource->getType() == type) {
								if(result.x < 0 || unit->getPos().dist(cache) < unit->getPos().dist(result)) {
									if(unit->isBadHarvestPos(cache) == false) {
										result = cache;
										// Close enough to our position, no more looking
										if(unit->getPos().dist(result) <= (harvestDistance * 2)) {
											foundCloseResource = true;
											break;
										}
									}
								}
							}
						}
						else {
							deleteList.push_back(cache);
						}
					}
					else {
						deleteList.push_back(cache);
					}
				}
			}

			if(deleteList.empty() == false) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"[cleaning old resource targets] deleteList.size() [" MG_SIZE_T_SPECIFIER "] cacheResourceTargetList.size() [" MG_SIZE_T_SPECIFIER "] result [%s]",
										deleteList.size(),cacheResourceTargetList.size(),result.getString().c_str());

					if(frameIndex < 0) {
						unit->logSynchData(__FILE__,__LINE__,szBuf);
					}
					else {
						unit->logSynchDataThreaded(__FILE__,__LINE__,szBuf);
					}
				}

				cleanupResourceTypeTargetCache(&deleteList,frameIndex);
			}
		}
	}

	return result;
}

// CANNOT MODIFY the cache here since the AI calls this method and the AI is only controlled
// by the server for network games and it would cause out of synch since clients do not call
// this method so DO NOT modify the cache here!
Vec2i Faction::getClosestResourceTypeTargetFromCache(const Vec2i &pos, const ResourceType *type) {
	Vec2i result(-1);
	if(cachingDisabled == false) {
		if(cacheResourceTargetList.empty() == false) {
			//std::vector<Vec2i> deleteList;

			const int harvestDistance = 5;
			const Map *map = world->getMap();

			bool foundCloseResource = false;

			// 0 means start looking leftbottom to top right
			int tryRadius = random.randRange(0,1);
			if(tryRadius == 0) {
				// First look immediately around the given position
				for(int j = -harvestDistance; j <= harvestDistance && foundCloseResource == false; ++j) {
					for(int k = -harvestDistance; k <= harvestDistance && foundCloseResource == false; ++k) {
						Vec2i newPos = pos + Vec2i(j,k);
						if(map->isInside(newPos) == true && isResourceTargetInCache(newPos) == false) {
							const SurfaceCell *sc = map->getSurfaceCell(map->toSurfCoords(newPos));
							if( sc != NULL && sc->getResource() != NULL) {
								const Resource *resource = sc->getResource();
								if(resource->getType() != NULL && resource->getType() == type) {
									if(result.x < 0 || pos.dist(newPos) < pos.dist(result)) {
										result = newPos;
										foundCloseResource = true;
										break;
									}
								}
							}
							//else {
							//	deleteList.push_back(newPos);
							//}
						}
					}
				}
			}
			else {
				// First look immediately around the given position
				for(int j = harvestDistance; j >= -harvestDistance && foundCloseResource == false; --j) {
					for(int k = harvestDistance; k >= -harvestDistance && foundCloseResource == false; --k) {
						Vec2i newPos = pos + Vec2i(j,k);
						if(map->isInside(newPos) == true && isResourceTargetInCache(newPos) == false) {
							const SurfaceCell *sc = map->getSurfaceCell(map->toSurfCoords(newPos));
							if( sc != NULL && sc->getResource() != NULL) {
								const Resource *resource = sc->getResource();
								if(resource->getType() != NULL && resource->getType() == type) {
									if(result.x < 0 || pos.dist(newPos) < pos.dist(result)) {
										result = newPos;
										foundCloseResource = true;
										break;
									}
								}
							}
							//else {
							//	deleteList.push_back(newPos);
							//}
						}
					}
				}
			}

			if(foundCloseResource == false) {
				// Now check the whole cache
				for(std::map<Vec2i,int>::iterator iter = cacheResourceTargetList.begin();
											  iter != cacheResourceTargetList.end() && foundCloseResource == false;
											  ++iter) {
					const Vec2i &cache = iter->first;
					if(map->isInside(cache) == true) {
						const SurfaceCell *sc = map->getSurfaceCell(map->toSurfCoords(cache));
						if( sc != NULL && sc->getResource() != NULL) {
							const Resource *resource = sc->getResource();
							if(resource->getType() != NULL && resource->getType() == type) {
								if(result.x < 0 || pos.dist(cache) < pos.dist(result)) {
									result = cache;
									// Close enough to our position, no more looking
									if(pos.dist(result) <= (harvestDistance * 2)) {
										foundCloseResource = true;
										break;
									}
								}
							}
						}
						//else {
						//	deleteList.push_back(cache);
						//}
					}
					//else {
					//	deleteList.push_back(cache);
					//}
				}
			}
		}
	}

	return result;
}

void Faction::cleanupResourceTypeTargetCache(std::vector<Vec2i> *deleteListPtr,int frameIndex) {
	if(cachingDisabled == false) {
		if(cacheResourceTargetList.empty() == false) {
			const int cleanupInterval = (GameConstants::updateFps * 5);
			bool needToCleanup = (getFrameCount() % cleanupInterval == 0);

			if(deleteListPtr != NULL || needToCleanup == true) {
				std::vector<Vec2i> deleteList;

				if(deleteListPtr != NULL) {
					deleteList = *deleteListPtr;
				}
				else {
					for(std::map<Vec2i,int>::iterator iter = cacheResourceTargetList.begin();
												  iter != cacheResourceTargetList.end(); ++iter) {
						const Vec2i &cache = iter->first;

						if(world->getMap()->getSurfaceCell(world->getMap()->toSurfCoords(cache)) != NULL) {
							Resource *resource = world->getMap()->getSurfaceCell(world->getMap()->toSurfCoords(cache))->getResource();
							if(resource == NULL) {
								deleteList.push_back(cache);
							}
						}
						else {
							deleteList.push_back(cache);
						}
					}
				}

				if(deleteList.empty() == false) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
						char szBuf[8096]="";
						snprintf(szBuf,8096,"[cleaning old resource targets] deleteList.size() [" MG_SIZE_T_SPECIFIER "] cacheResourceTargetList.size() [" MG_SIZE_T_SPECIFIER "], needToCleanup [%d]",
											deleteList.size(),cacheResourceTargetList.size(),needToCleanup);
						//unit->logSynchData(szBuf);

						char szBuf1[8096]="";
						snprintf(szBuf1,8096,"----------------------------------- START [%d] ------------------------------------------------\n",getFrameCount());
						string logDataText = szBuf1;

						snprintf(szBuf1,8096,"[%s::%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__);
						logDataText += szBuf1;

						snprintf(szBuf1,8096,"%s\n",szBuf);
						logDataText += szBuf1;

						snprintf(szBuf1,8096,"------------------------------------ END [%d] -------------------------------------------------\n",getFrameCount());
						logDataText += szBuf1;

						if(frameIndex < 0) {
							SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"%s",logDataText.c_str());
						}
						else {
							addWorldSynchThreadedLogList(logDataText);
						}
					}

					for(int i = 0; i < (int)deleteList.size(); ++i) {
						Vec2i &cache = deleteList[i];
						cacheResourceTargetList.erase(cache);
					}
				}
			}
		}
	}
}

//std::vector<Vec2i> Faction::findCachedPath(const Vec2i &target, Unit *unit) {
//	std::vector<Vec2i> result;
//	if(cachingDisabled == false) {
//		if(successfulPathFinderTargetList.find(target) == successfulPathFinderTargetList.end()) {
//			// Lets find the shortest and most successful path already taken by a
//			// similar sized unit
//
//			bool foundCachedPath = false;
//			std::vector<FactionPathSuccessCache> &cacheList = successfulPathFinderTargetList[target];
//			int unitSize = unit->getType()->getSize();
//			for(int i = 0; i < cacheList.size(); ++i) {
//				FactionPathSuccessCache &cache = cacheList[i];
//				if(cache.unitSize <= unitSize) {
//					vector<std::pair<vector<Vec2i>, int> > &pathQueue = cache.pathQueue;
//
//					for(int j = 0; j < pathQueue.size(); ++j) {
//						// Now start at the end of the path and see how many nodes
//						// until we reach a cell near the unit's current position
//						std::pair<vector<Vec2i>, int> &path = pathQueue[j];
//
//						for(int k = path.first.size() - 1; k >= 0; --k) {
//							if(world->getMap()->canMove(unit, unit->getPos(), path.first[k]) == true) {
//								if(foundCachedPath == false) {
//									for(int l = k; l < path.first.size(); ++l) {
//										result.push_back(path.first[l]);
//									}
//								}
//								else {
//									if(result.size() > (path.first.size() - k)) {
//										for(int l = k; l < path.first.size(); ++l) {
//											result.push_back(path.first[l]);
//										}
//									}
//								}
//								foundCachedPath = true;
//
//								break;
//							}
//						}
//					}
//				}
//			}
//		}
//	}
//
//	return result;
//}

//void Faction::addCachedPath(const Vec2i &target, Unit *unit) {
//	if(cachingDisabled == false) {
//		if(successfulPathFinderTargetList.find(target) == successfulPathFinderTargetList.end()) {
//			FactionPathSuccessCache cache;
//			cache.unitSize = unit->getType()->getSize();
//			cache.pathQueue.push_back(make_pair<vector<Vec2i>, int>(unit->getCurrentTargetPathTaken().second,1));
//			successfulPathFinderTargetList[target].push_back(cache);
//		}
//		else {
//			bool finishedAdd = false;
//			std::pair<Vec2i,std::vector<Vec2i> > currentTargetPathTaken = unit->getCurrentTargetPathTaken();
//			std::vector<FactionPathSuccessCache> &cacheList = successfulPathFinderTargetList[target];
//			int unitSize = unit->getType()->getSize();
//
//			for(int i = 0; i < cacheList.size() && finishedAdd == false; ++i) {
//				FactionPathSuccessCache &cache = cacheList[i];
//				if(cache.unitSize <= unitSize) {
//					vector<std::pair<vector<Vec2i>, int> > &pathQueue = cache.pathQueue;
//
//					for(int j = 0; j < pathQueue.size() && finishedAdd == false; ++j) {
//						// Now start at the end of the path and see how many nodes are the same
//						std::pair<vector<Vec2i>, int> &path = pathQueue[j];
//						int minPathSize = std::min(path.first.size(),currentTargetPathTaken.second.size());
//						int intersectIndex = -1;
//
//						for(int k = 0; k < minPathSize; ++k) {
//							if(path.first[path.first.size() - k - 1] != currentTargetPathTaken.second[currentTargetPathTaken.second.size() - k - 1]) {
//								intersectIndex = k;
//								break;
//							}
//						}
//
//						// New path is same or longer than old path so replace
//						// old path with new
//						if(intersectIndex + 1 == path.first.size()) {
//							path.first = currentTargetPathTaken.second;
//							path.second++;
//							finishedAdd = true;
//						}
//						// Old path is same or longer than new path so
//						// do nothing
//						else if(intersectIndex + 1 == currentTargetPathTaken.second.size()) {
//							path.second++;
//							finishedAdd = true;
//						}
//					}
//
//					// If new path is >= 10 cells add it
//					if(finishedAdd == false && currentTargetPathTaken.second.size() >= 10) {
//						pathQueue.push_back(make_pair<vector<Vec2i>, int>(currentTargetPathTaken.second,1));
//					}
//				}
//			}
//		}
//	}
//}

void Faction::deletePixels() {
	if(factionType != NULL) {
		factionType->deletePixels();
	}
}

Unit * Faction::findClosestUnitWithSkillClass(	const Vec2i &pos,const CommandClass &cmdClass,
												const std::vector<SkillClass> &skillClassList,
												const UnitType *unitType) {
	Unit *result = NULL;

/*
	std::map<CommandClass,std::map<int,int> >::iterator iterFind = cacheUnitCommandClassList.find(cmdClass);
	if(iterFind != cacheUnitCommandClassList.end()) {
		for(std::map<int,int>::iterator iter = iterFind->second.begin();
				iter != iterFind->second.end(); ++iter) {
			Unit *curUnit = findUnit(iter->second);
			if(curUnit != NULL) {

				const CommandType *cmdType = curUnit->getType()->getFirstCtOfClass(cmdClass);
				bool isUnitPossibleCandidate = (cmdType != NULL);
				if(skillClassList.empty() == false) {
					isUnitPossibleCandidate = false;

					for(int j = 0; j < skillClassList.size(); ++j) {
						SkillClass skValue = skillClassList[j];
						if(curUnit->getCurrSkill()->getClass() == skValue) {
							isUnitPossibleCandidate = true;
							break;
						}
					}
				}

				if(isUnitPossibleCandidate == true) {
					if(result == NULL || curUnit->getPos().dist(pos) < result->getPos().dist(pos)) {
						result = curUnit;
					}
				}
			}
		}
	}
*/

	if(result == NULL) {
		for(int i = 0; i < getUnitCount(); ++i) {
			Unit *curUnit = getUnit(i);

			bool isUnitPossibleCandidate = false;

			const CommandType *cmdType = curUnit->getType()->getFirstCtOfClass(cmdClass);
			if(cmdType != NULL) {
				const RepairCommandType *rct = dynamic_cast<const RepairCommandType *>(cmdType);
				if(rct != NULL && rct->isRepairableUnitType(unitType)) {
					isUnitPossibleCandidate = true;
				}
			}
			else {
				isUnitPossibleCandidate = false;
			}

			if(isUnitPossibleCandidate == true && skillClassList.empty() == false) {
				isUnitPossibleCandidate = false;

				for(int j = 0; j < (int)skillClassList.size(); ++j) {
					SkillClass skValue = skillClassList[j];
					if(curUnit->getCurrSkill()->getClass() == skValue) {
						isUnitPossibleCandidate = true;
						break;
					}
				}
			}


			if(isUnitPossibleCandidate == true) {
				//cacheUnitCommandClassList[cmdClass][curUnit->getId()] = curUnit->getId();

				if(result == NULL || curUnit->getPos().dist(pos) < result->getPos().dist(pos)) {
					result = curUnit;
				}
			}
		}
	}
	return result;
}

int Faction::getFrameCount() {
	int frameCount = 0;
	const Game *game = Renderer::getInstance().getGame();
	if(game != NULL && game->getWorld() != NULL) {
		frameCount = game->getWorld()->getFrameCount();
	}

	return frameCount;
}

const SwitchTeamVote * Faction::getFirstSwitchTeamVote() const {
	const SwitchTeamVote *vote = NULL;
	if(switchTeamVotes.empty() == false) {
		for(std::map<int,SwitchTeamVote>::const_iterator iterMap = switchTeamVotes.begin();
				iterMap != switchTeamVotes.end(); ++iterMap) {
			const SwitchTeamVote &curVote = iterMap->second;
			if(curVote.voted == false) {
				vote = &curVote;
				break;
			}
		}
	}

	return vote;
}

SwitchTeamVote * Faction::getSwitchTeamVote(int factionIndex) {
	SwitchTeamVote *vote = NULL;
	if(switchTeamVotes.find(factionIndex) != switchTeamVotes.end()) {
		vote = &switchTeamVotes[factionIndex];
	}

	return vote;
}

void Faction::setSwitchTeamVote(SwitchTeamVote &vote) {
	switchTeamVotes[vote.factionIndex] = vote;
}

bool Faction::canCreateUnit(const UnitType *ut, bool checkBuild, bool checkProduce, bool checkMorph) const {
	// Now check that at least 1 other unit can produce, build or morph this unit
	bool foundUnit = false;
	for(int l = 0; l < this->getUnitCount() && foundUnit == false; ++l) {
		const UnitType *unitType2 = this->getUnit(l)->getType();

		for(int j = 0; j < unitType2->getCommandTypeCount() && foundUnit == false; ++j) {
			const CommandType *cmdType = unitType2->getCommandType(j);
			if(cmdType != NULL) {
				// Check if this is a produce command
				if(checkProduce == true && cmdType->getClass() == ccProduce) {
					const ProduceCommandType *produce = dynamic_cast<const ProduceCommandType *>(cmdType);
					if(produce != NULL) {
						const UnitType *produceUnit = produce->getProducedUnit();

						if( produceUnit != NULL &&
							ut->getId() != unitType2->getId() &&
							ut->getName(false) == produceUnit->getName(false)) {
							 foundUnit = true;
							 break;
						}
					}
				}
				// Check if this is a build command
				else if(checkBuild == true && cmdType->getClass() == ccBuild) {
					const BuildCommandType *build = dynamic_cast<const BuildCommandType *>(cmdType);
					if(build != NULL) {
						for(int k = 0; k < build->getBuildingCount() && foundUnit == false; ++k) {
							const UnitType *buildUnit = build->getBuilding(k);

							if( buildUnit != NULL &&
								ut->getId() != unitType2->getId() &&
								ut->getName(false) == buildUnit->getName(false)) {
								 foundUnit = true;
								 break;
							}
						}
					}
				}
				// Check if this is a morph command
				else if(checkMorph == true && cmdType->getClass() == ccMorph) {
					const MorphCommandType *morph = dynamic_cast<const MorphCommandType *>(cmdType);
					if(morph != NULL) {
						const UnitType *morphUnit = morph->getMorphUnit();

						if( morphUnit != NULL &&
							ut->getId() != unitType2->getId() &&
							ut->getName(false) == morphUnit->getName(false)) {
							 foundUnit = true;
							 break;
						}
					}
				}
			}
		}
	}

	return foundUnit;
}

void Faction::clearCaches() {
	cacheResourceTargetList.clear();
	cachedCloseResourceTargetLookupList.clear();

	//aliveUnitListCache.clear();
	//mobileUnitListCache.clear();
	//beingBuiltUnitListCache.clear();

	unsigned int unitCount = this->getUnitCount();
	for(unsigned int i = 0; i < unitCount; ++i) {
		Unit *unit = this->getUnit(i);
		if(unit != NULL) {
			unit->clearCaches();
		}
	}
}

uint64 Faction::getCacheKBytes(uint64 *cache1Size, uint64 *cache2Size, uint64 *cache3Size, uint64 *cache4Size, uint64 *cache5Size) {
	uint64 cache1Count = 0;
	uint64 cache2Count = 0;
	uint64 cache3Count = 0;
	uint64 cache4Count = 0;
	uint64 cache5Count = 0;

	for(std::map<Vec2i,int>::iterator iterMap1 = cacheResourceTargetList.begin();
		iterMap1 != cacheResourceTargetList.end(); ++iterMap1) {
		cache1Count++;
	}
	for(std::map<Vec2i,bool>::iterator iterMap1 = cachedCloseResourceTargetLookupList.begin();
		iterMap1 != cachedCloseResourceTargetLookupList.end(); ++iterMap1) {
		cache2Count++;
	}
	for(std::map<int,const Unit *>::iterator iterMap1 = aliveUnitListCache.begin();
		iterMap1 != aliveUnitListCache.end(); ++iterMap1) {
		cache3Count++;
	}
	for(std::map<int,const Unit *>::iterator iterMap1 = mobileUnitListCache.begin();
		iterMap1 != mobileUnitListCache.end(); ++iterMap1) {
		cache4Count++;
	}
	for(std::map<int,const Unit *>::iterator iterMap1 = beingBuiltUnitListCache.begin();
		iterMap1 != beingBuiltUnitListCache.end(); ++iterMap1) {
		cache5Count++;
	}

	if(cache1Size) {
		*cache1Size = cache1Count;
	}
	if(cache2Size) {
		*cache2Size = cache2Count;
	}
	if(cache3Size) {
		*cache3Size = cache3Count;
	}
	if(cache4Size) {
		*cache4Size = cache4Count;
	}
	if(cache5Size) {
		*cache5Size = cache5Count;
	}

	uint64 totalBytes = cache1Count * sizeof(int);
	totalBytes += cache2Count * sizeof(bool);
	totalBytes += cache3Count * (sizeof(int) + sizeof(const Unit *));
	totalBytes += cache4Count * (sizeof(int) + sizeof(const Unit *));
	totalBytes += cache5Count * (sizeof(int) + sizeof(const Unit *));

	totalBytes /= 1000;

	return totalBytes;
}

string Faction::getCacheStats() {
	string result = "";

	int cache1Count = 0;
	int cache2Count = 0;

	for(std::map<Vec2i,int>::iterator iterMap1 = cacheResourceTargetList.begin();
		iterMap1 != cacheResourceTargetList.end(); ++iterMap1) {
		cache1Count++;
	}
	for(std::map<Vec2i,bool>::iterator iterMap1 = cachedCloseResourceTargetLookupList.begin();
		iterMap1 != cachedCloseResourceTargetLookupList.end(); ++iterMap1) {
		cache2Count++;
	}

	uint64 totalBytes = cache1Count * sizeof(int);
	totalBytes += cache2Count * sizeof(bool);

	totalBytes /= 1000;

	char szBuf[8096]="";
	snprintf(szBuf,8096,"cache1Count [%d] cache2Count [%d] total KB: %s",cache1Count,cache2Count,formatNumber(totalBytes).c_str());
	result = szBuf;
	return result;
}

std::string Faction::toString(bool crcMode) const {
	std::string result = "FactionIndex = " + intToStr(this->index) + "\n";
    result += "teamIndex = " + intToStr(this->teamIndex) + "\n";
    result += "startLocationIndex = " + intToStr(this->startLocationIndex) + "\n";
    if(crcMode == false) {
    	result += "thisFaction = " + intToStr(this->thisFaction) + "\n";
    	result += "control = " + intToStr(this->control) + "\n";
    }

    if(this->factionType != NULL) {
    	result += this->factionType->toString() + "\n";
    }

	result += this->upgradeManager.toString() + "\n";

	result += "ResourceCount = " + intToStr(resources.size()) + "\n";
	for(int idx = 0; idx < (int)resources.size(); idx ++) {
		result += "index = " + intToStr(idx) + " " + resources[idx].toString() + "\n";
	}

	result += "StoreCount = " + intToStr(store.size()) + "\n";
	for(int idx = 0; idx < (int)store.size(); idx ++) {
		result += "index = " + intToStr(idx) + " " + store[idx].toString()  + "\n";
	}

	result += "Allies = " + intToStr(allies.size()) + "\n";
	for(int idx = 0; idx < (int)allies.size(); idx ++) {
		result += "index = " + intToStr(idx) + " name: " + allies[idx]->factionType->getName(false) + " factionindex = " + intToStr(allies[idx]->index)  + "\n";
	}

	result += "Units = " + intToStr(units.size()) + "\n";
	for(int idx = 0; idx < (int)units.size(); idx ++) {
		result += units[idx]->toString(crcMode) + "\n";
	}

	return result;
}

void Faction::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *factionNode = rootNode->addChild("Faction");

	upgradeManager.saveGame(factionNode);
	for(unsigned int i = 0; i < resources.size(); ++i) {
		Resource &resource = resources[i];
		resource.saveGame(factionNode);
	}
	XmlNode *storeNode = factionNode->addChild("Store");
	for(unsigned int i = 0; i < store.size(); ++i) {
		Resource &resource = store[i];
		resource.saveGame(storeNode);
	}

	for(unsigned int i = 0; i < allies.size(); ++i) {
		Faction *ally = allies[i];
		XmlNode *allyNode = factionNode->addChild("Ally");
		allyNode->addAttribute("allyFactionIndex",intToStr(ally->getIndex()), mapTagReplacements);
	}
	for(unsigned int i = 0; i < units.size(); ++i) {
		Unit *unit = units[i];
		unit->saveGame(factionNode);
	}

	factionNode->addAttribute("control",intToStr(control), mapTagReplacements);

	factionNode->addAttribute("overridePersonalityType",intToStr(overridePersonalityType), mapTagReplacements);
	factionNode->addAttribute("factiontype",factionType->getName(false), mapTagReplacements);
	factionNode->addAttribute("index",intToStr(index), mapTagReplacements);
	factionNode->addAttribute("teamIndex",intToStr(teamIndex), mapTagReplacements);
	factionNode->addAttribute("startLocationIndex",intToStr(startLocationIndex), mapTagReplacements);
	factionNode->addAttribute("thisFaction",intToStr(thisFaction), mapTagReplacements);

	for(std::map<Vec2i,int>::iterator iterMap = cacheResourceTargetList.begin();
			iterMap != cacheResourceTargetList.end(); ++iterMap) {
		XmlNode *cacheResourceTargetListNode = factionNode->addChild("cacheResourceTargetList");

		cacheResourceTargetListNode->addAttribute("key",iterMap->first.getString(), mapTagReplacements);
		cacheResourceTargetListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
	}

	for(std::map<Vec2i,bool>::iterator iterMap = cachedCloseResourceTargetLookupList.begin();
			iterMap != cachedCloseResourceTargetLookupList.end(); ++iterMap) {
		XmlNode *cachedCloseResourceTargetLookupListNode = factionNode->addChild("cachedCloseResourceTargetLookupList");

		cachedCloseResourceTargetLookupListNode->addAttribute("key",iterMap->first.getString(), mapTagReplacements);
		cachedCloseResourceTargetLookupListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
	}

	factionNode->addAttribute("random",intToStr(random.getLastNumber()), mapTagReplacements);
	factionNode->addAttribute("currentSwitchTeamVoteFactionIndex",intToStr(currentSwitchTeamVoteFactionIndex), mapTagReplacements);
	factionNode->addAttribute("allowSharedTeamUnits",intToStr(allowSharedTeamUnits), mapTagReplacements);

	for(std::set<const UnitType*>::iterator iterMap = lockedUnits.begin();
			iterMap != lockedUnits.end(); ++iterMap) {
		XmlNode *lockedUnitsListNode = factionNode->addChild("lockedUnitList");
		const UnitType *ut=*iterMap;

		lockedUnitsListNode->addAttribute("value",ut->getName(false), mapTagReplacements);
	}

	for(std::map<int,int>::iterator iterMap = unitsMovingList.begin();
			iterMap != unitsMovingList.end(); ++iterMap) {
		XmlNode *unitsMovingListNode = factionNode->addChild("unitsMovingList");

		unitsMovingListNode->addAttribute("key",intToStr(iterMap->first), mapTagReplacements);
		unitsMovingListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
	}

	for(std::map<int,int>::iterator iterMap = unitsPathfindingList.begin();
			iterMap != unitsPathfindingList.end(); ++iterMap) {
		XmlNode *unitsPathfindingListNode = factionNode->addChild("unitsPathfindingList");

		unitsPathfindingListNode->addAttribute("key",intToStr(iterMap->first), mapTagReplacements);
		unitsPathfindingListNode->addAttribute("value",intToStr(iterMap->second), mapTagReplacements);
	}
}

void Faction::loadGame(const XmlNode *rootNode, int factionIndex,GameSettings *settings,World *world) {
	XmlNode *factionNode = NULL;
	vector<XmlNode *> factionNodeList = rootNode->getChildList("Faction");
	for(unsigned int i = 0; i < factionNodeList.size(); ++i) {
		XmlNode *node = factionNodeList[i];
		if(node->getAttribute("index")->getIntValue() == factionIndex) {
			factionNode = node;
			break;
		}
	}

	if(factionNode != NULL) {

		allies.clear();
		vector<XmlNode *> allyNodeList = factionNode->getChildList("Ally");
		for(unsigned int i = 0; i < allyNodeList.size(); ++i) {
			XmlNode *allyNode = allyNodeList[i];

			int allyFactionIndex = allyNode->getAttribute("allyFactionIndex")->getIntValue();
			allies.push_back(world->getFaction(allyFactionIndex));
		}

		vector<XmlNode *> unitNodeList = factionNode->getChildList("Unit");
		for(unsigned int i = 0; i < unitNodeList.size(); ++i) {
			XmlNode *unitNode = unitNodeList[i];
			Unit *unit = Unit::loadGame(unitNode,settings,this, world);
			this->addUnit(unit);
		}

		for(unsigned int i = 0; i < resources.size(); ++i) {
			Resource &resource = resources[i];
			resource.loadGame(factionNode,i,techTree);
		}
		XmlNode *storeNode = factionNode->getChild("Store");
		for(unsigned int i = 0; i < store.size(); ++i) {
			Resource &resource = store[i];
			resource.loadGame(storeNode,i,techTree);
		}

		upgradeManager.loadGame(factionNode,this);

		control = static_cast<ControlType>(factionNode->getAttribute("control")->getIntValue());

		if(factionNode->hasAttribute("overridePersonalityType") == true) {
			overridePersonalityType = static_cast<FactionPersonalityType>(factionNode->getAttribute("overridePersonalityType")->getIntValue());
		}

		teamIndex = factionNode->getAttribute("teamIndex")->getIntValue();

		startLocationIndex = factionNode->getAttribute("startLocationIndex")->getIntValue();

		thisFaction = factionNode->getAttribute("thisFaction")->getIntValue() != 0;

		if(factionNode->hasAttribute("allowSharedTeamUnits") == true) {
			allowSharedTeamUnits = factionNode->getAttribute("allowSharedTeamUnits")->getIntValue() != 0;
		}

		vector<XmlNode *> cacheResourceTargetListNodeList = factionNode->getChildList("cacheResourceTargetList");
		for(unsigned int i = 0; i < cacheResourceTargetListNodeList.size(); ++i) {
			XmlNode *cacheResourceTargetListNode = cacheResourceTargetListNodeList[i];

			Vec2i vec = Vec2i::strToVec2(cacheResourceTargetListNode->getAttribute("key")->getValue());
			cacheResourceTargetList[vec] = cacheResourceTargetListNode->getAttribute("value")->getIntValue();
		}
		vector<XmlNode *> cachedCloseResourceTargetLookupListNodeList = factionNode->getChildList("cachedCloseResourceTargetLookupList");
		for(unsigned int i = 0; i < cachedCloseResourceTargetLookupListNodeList.size(); ++i) {
			XmlNode *cachedCloseResourceTargetLookupListNode = cachedCloseResourceTargetLookupListNodeList[i];

			Vec2i vec = Vec2i::strToVec2(cachedCloseResourceTargetLookupListNode->getAttribute("key")->getValue());
			cachedCloseResourceTargetLookupList[vec] = cachedCloseResourceTargetLookupListNode->getAttribute("value")->getIntValue() != 0;
		}

		random.setLastNumber(factionNode->getAttribute("random")->getIntValue());

		vector<XmlNode *> lockedUnitsListNodeList = factionNode->getChildList("lockedUnitList");
		for(unsigned int i = 0; i < lockedUnitsListNodeList.size(); ++i) {
			XmlNode *lockedUnitsListNode = lockedUnitsListNodeList[i];

			string unitName = lockedUnitsListNode->getAttribute("value")->getValue();
			lockedUnits.insert(getType()->getUnitType(unitName));
		}

		vector<XmlNode *> unitsMovingListNodeList = factionNode->getChildList("unitsMovingList");
		for(unsigned int i = 0; i < unitsMovingListNodeList.size(); ++i) {
			XmlNode *unitsMovingListNode = unitsMovingListNodeList[i];

			int unitId = unitsMovingListNode->getAttribute("key")->getIntValue();
			unitsMovingList[unitId] = unitsMovingListNode->getAttribute("value")->getIntValue();
		}
		vector<XmlNode *> unitsPathfindingListNodeList = factionNode->getChildList("unitsPathfindingList");
		for(unsigned int i = 0; i < unitsPathfindingListNodeList.size(); ++i) {
			XmlNode *unitsPathfindingListNode = unitsPathfindingListNodeList[i];

			int unitId = unitsPathfindingListNode->getAttribute("key")->getIntValue();
			unitsPathfindingList[unitId] = unitsPathfindingListNode->getAttribute("value")->getIntValue();
		}
	}
}

Checksum Faction::getCRC() {
	const bool consoleDebug = false;

	Checksum crcForFaction;

	// UpgradeManager upgradeManager;

	for(unsigned int i = 0; i < resources.size(); ++i) {
		Resource &resource = resources[i];
		//crcForFaction.addSum(resource.getCRC().getSum());
		uint32 crc = resource.getCRC().getSum();
		crcForFaction.addBytes(&crc,sizeof(uint32));
	}

	if(consoleDebug) {
		if(getWorld()->getFrameCount() % 40 == 0) {
			printf("#1 Frame #: %d Faction: %d CRC: %u\n",getWorld()->getFrameCount(),index,crcForFaction.getSum());
		}
	}

	for(unsigned int i = 0; i < store.size(); ++i) {
		Resource &resource = store[i];
		//crcForFaction.addSum(resource.getCRC().getSum());
		uint32 crc = resource.getCRC().getSum();
		crcForFaction.addBytes(&crc,sizeof(uint32));
	}

	if(consoleDebug) {
		if(getWorld()->getFrameCount() % 40 == 0) {
			printf("#2 Frame #: %d Faction: %d CRC: %u\n",getWorld()->getFrameCount(),index,crcForFaction.getSum());
		}
	}

	for(unsigned int i = 0; i < units.size(); ++i) {
		Unit *unit = units[i];
		//crcForFaction.addSum(unit->getCRC().getSum());
		uint32 crc = unit->getCRC().getSum();
		crcForFaction.addBytes(&crc,sizeof(uint32));
	}

	if(consoleDebug) {
		if(getWorld()->getFrameCount() % 40 == 0) {
			printf("#3 Frame #: %d Faction: %d CRC: %u\n",getWorld()->getFrameCount(),index,crcForFaction.getSum());
		}
	}

	return crcForFaction;
}

void Faction::addCRC_DetailsForWorldFrame(int worldFrameCount,bool isNetworkServer) {
	unsigned int MAX_FRAME_CACHE = 250;
	if(isNetworkServer == true) {
		MAX_FRAME_CACHE += 250;
	}
	crcWorldFrameDetails[worldFrameCount] = this->toString(true);
	//if(worldFrameCount <= 0) printf("Adding world frame: %d log entries: %lld\n",worldFrameCount,(long long int)crcWorldFrameDetails.size());

	for(unsigned int i = 0; i < units.size(); ++i) {
		Unit *unit = units[i];

		unit->getRandom()->clearLastCaller();
		unit->clearNetworkCRCDecHpList();
		unit->clearParticleInfo();
	}

	if((unsigned int)crcWorldFrameDetails.size() > MAX_FRAME_CACHE) {
		//printf("===> Removing older world frame log entries: %lld\n",(long long int)crcWorldFrameDetails.size());

		for(;(unsigned int)crcWorldFrameDetails.size() - MAX_FRAME_CACHE > 0;) {
			crcWorldFrameDetails.erase(crcWorldFrameDetails.begin());
		}
	}
}

string Faction::getCRC_DetailsForWorldFrame(int worldFrameCount) {
	if(crcWorldFrameDetails.empty()) {
		return "";
	}
	return crcWorldFrameDetails[worldFrameCount];
}

std::pair<int,string> Faction::getCRC_DetailsForWorldFrameIndex(int worldFrameIndex) const {
	if(crcWorldFrameDetails.empty()) {
		return make_pair<int,string>(0,"");
	}
	std::map<int,string>::const_iterator iterMap = crcWorldFrameDetails.begin();
	std::advance( iterMap, worldFrameIndex );
	if(iterMap == crcWorldFrameDetails.end()) {
		return make_pair<int,string>(0,"");
	}
	return std::pair<int,string>(iterMap->first,iterMap->second);
}

string Faction::getCRC_DetailsForWorldFrames() const {
	string result = "";
	for(std::map<int,string>::const_iterator iterMap = crcWorldFrameDetails.begin();
			iterMap != crcWorldFrameDetails.end(); ++iterMap) {
		result += string("============================================================================\n");
		result += string("** world frame: ") + intToStr(iterMap->first) + string(" detail: ") + iterMap->second;
	}
	return result;
}

uint64 Faction::getCRC_DetailsForWorldFrameCount() const {
	return crcWorldFrameDetails.size();
}

}}//end namespace
