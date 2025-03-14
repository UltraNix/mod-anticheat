/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AnticheatMgr.h"
#include "MapManager.h"
#include "Player.h"
#include "Configuration/Config.h"
#define CLIMB_ANGLE 1.9f

AnticheatMgr::AnticheatMgr()
{
}

AnticheatMgr::~AnticheatMgr()
{
	m_Players.clear();
}

void AnticheatMgr::JumpHackDetection(Player* player, MovementInfo /* movementInfo */, uint32 opcode)
{
	if (!sConfigMgr->GetBoolDefault("Anticheat.DetectJumpHack", true))
		return;

	uint32 key = player->GetGUIDLow();

	if (m_Players[key].GetLastOpcode() == MSG_MOVE_JUMP && opcode == MSG_MOVE_JUMP)
	{
		BuildReport(player, JUMP_HACK_REPORT);
		sLog->outString("AnticheatMgr:: Jump-Hack detected player %s (%u)", player->GetName().c_str(), player->GetGUIDLow());
	}
}

void AnticheatMgr::WalkOnWaterHackDetection(Player* player, MovementInfo  movementInfo)
{
	if (!sConfigMgr->GetBoolDefault("Anticheat.DetectWaterWalkHack", true))
		return;

	uint32 key = player->GetGUIDLow();
	/* Thanks to @LilleCarl */
	if (!m_Players[key].GetLastMovementInfo().HasMovementFlag(MOVEMENTFLAG_WATERWALKING) && !movementInfo.HasMovementFlag(MOVEMENTFLAG_WATERWALKING))
		return;

	// if we are a ghost we can walk on water
	if (!player->IsAlive())
		return;

	if (player->HasAuraType(SPELL_AURA_FEATHER_FALL) ||
		player->HasAuraType(SPELL_AURA_SAFE_FALL) ||
		player->HasAuraType(SPELL_AURA_WATER_WALK))
		return;
	if (sConfigMgr->GetBoolDefault("Anticheat.KickPlayerWaterWalkHack", false))
	{
		if (sConfigMgr->GetBoolDefault("Anticheat.WriteLog", false))
			sLog->outString("AnticheatMgr:: Walk on Water - Hack detected and counteracted by kicking player %s (%u)", player->GetName().c_str(), player->GetGUIDLow());

		player->GetSession()->KickPlayer(true);
		if(sConfigMgr->GetBoolDefault("Anticheat.AnnounceKick", true))
		{
			std::string plr = player->GetName();
			std::string tag_colour = "7bbef7";
			std::string plr_colour = "ff0000";
			std::ostringstream stream;
			stream << "|CFF" << plr_colour << "[AntiCheat]|r|CFF" << tag_colour <<
				" Player |r|cff" << plr_colour <<  plr << "|r|cff" << tag_colour <<
				" has been kicked.|r";
			sWorld->SendServerMessage(SERVER_MSG_STRING, stream.str().c_str());
		}
	}
	else if (sConfigMgr->GetBoolDefault("Anticheat.WriteLog", false)) {
		sLog->outString("AnticheatMgr:: Walk on Water - Hack detected player %s (%u)", player->GetName().c_str(), player->GetGUIDLow());
	}
	BuildReport(player, WALK_WATER_HACK_REPORT);

}

void AnticheatMgr::FlyHackDetection(Player* player, MovementInfo  movementInfo)
{
    if (!sConfigMgr->GetBoolDefault("Anticheat.DetectFlyHack", true))
    {
        return;
    }

    if (player->HasAuraType(SPELL_AURA_FLY) || player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) || player->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED))//overkill but wth
    {
        return;
    }

    /*Thanks to @LilleCarl for info to check extra flag*/
    bool stricterChecks = true;
    if (sConfigMgr->GetBoolDefault("Anticheat.StricterFlyHackCheck", false))
    {
        stricterChecks = !(movementInfo.HasMovementFlag(MOVEMENTFLAG_ASCENDING) && !player->IsInWater());
    }

    if (!movementInfo.HasMovementFlag(MOVEMENTFLAG_CAN_FLY) && !movementInfo.HasMovementFlag(MOVEMENTFLAG_FLYING) && stricterChecks)
    {
        return;
    }

    if (sConfigMgr->GetBoolDefault("Anticheat.KickPlayerFlyHack", false))
    {
        if (sConfigMgr->GetBoolDefault("Anticheat.WriteLog", false))
        {
            sLog->outString("AnticheatMgr:: Fly-Hack detected and counteracted by kicking player %s (%u)", player->GetName().c_str(), player->GetGUIDLow());
        }

        player->GetSession()->KickPlayer(true);
        if(sConfigMgr->GetBoolDefault("Anticheat.AnnounceKick", true))
        {
            std::string plr = player->GetName();
            std::string tag_colour = "7bbef7";
            std::string plr_colour = "ff0000";
            std::ostringstream stream;
            stream << "|CFF" << plr_colour << "[AntiCheat]|r|CFF" << tag_colour <<
                " Player |r|cff" << plr_colour <<  plr << "|r|cff" << tag_colour <<
                " has been kicked.|r";
            sWorld->SendServerMessage(SERVER_MSG_STRING, stream.str().c_str());
        }
    } else if (sConfigMgr->GetBoolDefault("Anticheat.WriteLog", false)) 
      {
        sLog->outString( "AnticheatMgr:: Fly-Hack detected player %s (%u)", player->GetName().c_str(), player->GetGUIDLow());
      }

    BuildReport(player,FLY_HACK_REPORT);
}

void AnticheatMgr::TeleportPlaneHackDetection(Player* player, MovementInfo movementInfo)
{
	if (!sConfigMgr->GetBoolDefault("Anticheat.DetectTelePlaneHack", true))
		return;

	uint32 key = player->GetGUIDLow();

	if (m_Players[key].GetLastMovementInfo().pos.GetPositionZ() != 0 ||
		movementInfo.pos.GetPositionZ() != 0)
		return;

	if (movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING))
		return;

	// DEAD_FALLING was deprecated
	//if (player->getDeathState() == DEAD_FALLING)
	//    return;
	float x, y, z;
	player->GetPosition(x, y, z);
	float ground_Z = player->GetMap()->GetHeight(x, y, z);
	float z_diff = fabs(ground_Z - z);

	// we are not really walking there
	if (z_diff > 1.0f)
	{
		if (sConfigMgr->GetBoolDefault("Anticheat.WriteLog", false))
			sLog->outString("AnticheatMgr:: Teleport To Plane - Hack detected player %s (%u)", player->GetName().c_str(), player->GetGUIDLow());

		BuildReport(player, TELEPORT_PLANE_HACK_REPORT);
	}
}

void AnticheatMgr::StartHackDetection(Player* player, MovementInfo movementInfo, uint32 opcode)
{
	if (!sConfigMgr->GetBoolDefault("Anticheat.Enabled", 0))
		return;

	if (player->IsGameMaster())
		return;

	uint32 key = player->GetGUIDLow();

	if (player->IsInFlight() || player->GetTransport() || player->GetVehicle())
	{
		m_Players[key].SetLastMovementInfo(movementInfo);
		m_Players[key].SetLastOpcode(opcode);
		return;
	}

	SpeedHackDetection(player, movementInfo);
	FlyHackDetection(player, movementInfo);
	WalkOnWaterHackDetection(player, movementInfo);
	JumpHackDetection(player, movementInfo, opcode);
	TeleportPlaneHackDetection(player, movementInfo);
	ClimbHackDetection(player, movementInfo, opcode);

	m_Players[key].SetLastMovementInfo(movementInfo);
	m_Players[key].SetLastOpcode(opcode);
}

// basic detection
void AnticheatMgr::ClimbHackDetection(Player *player, MovementInfo movementInfo, uint32 opcode)
{
	if (!sConfigMgr->GetBoolDefault("Anticheat.DetectClimbHack", false))
		return;

	uint32 key = player->GetGUIDLow();

	if (opcode != MSG_MOVE_HEARTBEAT ||
		m_Players[key].GetLastOpcode() != MSG_MOVE_HEARTBEAT)
		return;

	// in this case we don't care if they are "legal" flags, they are handled in another parts of the Anticheat Manager.
	if (player->IsInWater() ||
		player->IsFlying() ||
		player->IsFalling())
		return;

	Position playerPos;
	// Position pos = player->GetPosition();

	float deltaZ = fabs(playerPos.GetPositionZ() - movementInfo.pos.GetPositionZ());
	float deltaXY = movementInfo.pos.GetExactDist2d(&playerPos);

	float angle = Position::NormalizeOrientation(tan(deltaZ / deltaXY));

	if (angle > CLIMB_ANGLE)
	{
		if (sConfigMgr->GetBoolDefault("Anticheat.WriteLog", false))
			sLog->outString("AnticheatMgr:: Climb-Hack detected player %s (%u)", player->GetName().c_str(), player->GetGUIDLow());

		BuildReport(player, CLIMB_HACK_REPORT);
	}
}

void AnticheatMgr::SpeedHackDetection(Player* player, MovementInfo movementInfo)
{
	if (!sConfigMgr->GetBoolDefault("Anticheat.DetectSpeedHack", true))
		return;

	uint32 key = player->GetGUIDLow();

	// We also must check the map because the movementFlag can be modified by the client.
	// If we just check the flag, they could always add that flag and always skip the speed hacking detection.
	// 369 == DEEPRUN TRAM
	if (m_Players[key].GetLastMovementInfo().HasMovementFlag(MOVEMENTFLAG_ONTRANSPORT) && player->GetMapId() == 369)
		return;

	uint32 distance2D = (uint32)movementInfo.pos.GetExactDist2d(&m_Players[key].GetLastMovementInfo().pos);
	uint8 moveType = 0;

	// we need to know HOW is the player moving
	// TO-DO: Should we check the incoming movement flags?
	if (player->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
		moveType = MOVE_SWIM;
	else if (player->IsFlying())
		moveType = MOVE_FLIGHT;
	else if (player->HasUnitMovementFlag(MOVEMENTFLAG_WALKING))
		moveType = MOVE_WALK;
	else
		moveType = MOVE_RUN;

	// how many yards the player can do in one sec.
	uint32 speedRate = (uint32)(player->GetSpeed(UnitMoveType(moveType)) + movementInfo.jump.xyspeed);

	// how long the player took to move to here.
	uint32 timeDiff = getMSTimeDiff(m_Players[key].GetLastMovementInfo().time, movementInfo.time);

	if (!timeDiff)
		timeDiff = 1;

	// this is the distance doable by the player in 1 sec, using the time done to move to this point.
	uint32 clientSpeedRate = distance2D * 1000 / timeDiff;

	// we did the (uint32) cast to accept a margin of tolerance
	if (clientSpeedRate > speedRate)
	{
		if (sConfigMgr->GetBoolDefault("Anticheat.WriteLog", false))
			sLog->outString("AnticheatMgr:: Speed-Hack detected player %s (%u)", player->GetName().c_str(), player->GetGUIDLow());

		BuildReport(player, SPEED_HACK_REPORT);
	}
}


void AnticheatMgr::HandlePlayerLogin(Player* player)
{
	// we must delete this to prevent errors in case of crash
	CharacterDatabase.PExecute("DELETE FROM players_reports_status WHERE guid=%u", player->GetGUIDLow());
	// we initialize the pos of lastMovementPosition var.
	m_Players[player->GetGUIDLow()].SetPosition(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation());
	QueryResult resultDB = CharacterDatabase.PQuery("SELECT * FROM daily_players_reports WHERE guid=%u;", player->GetGUIDLow());

	if (resultDB)
		m_Players[player->GetGUIDLow()].SetDailyReportState(true);
}

void AnticheatMgr::HandlePlayerLogout(Player* player)
{
	// TO-DO Make a table that stores the cheaters of the day, with more detailed information.

	// We must also delete it at logout to prevent have data of offline players in the db when we query the database (IE: The GM Command)
	CharacterDatabase.PExecute("DELETE FROM players_reports_status WHERE guid=%u", player->GetGUIDLow());
	// Delete not needed data from the memory.
	m_Players.erase(player->GetGUIDLow());
}

void AnticheatMgr::SavePlayerData(Player* player)
{
	CharacterDatabase.PExecute("REPLACE INTO players_reports_status (guid,average,total_reports,speed_reports,fly_reports,jump_reports,waterwalk_reports,teleportplane_reports,climb_reports,creation_time) VALUES (%u,%f,%u,%u,%u,%u,%u,%u,%u,%u);", player->GetGUIDLow(), m_Players[player->GetGUIDLow()].GetAverage(), m_Players[player->GetGUIDLow()].GetTotalReports(), m_Players[player->GetGUIDLow()].GetTypeReports(SPEED_HACK_REPORT), m_Players[player->GetGUIDLow()].GetTypeReports(FLY_HACK_REPORT), m_Players[player->GetGUIDLow()].GetTypeReports(JUMP_HACK_REPORT), m_Players[player->GetGUIDLow()].GetTypeReports(WALK_WATER_HACK_REPORT), m_Players[player->GetGUIDLow()].GetTypeReports(TELEPORT_PLANE_HACK_REPORT), m_Players[player->GetGUIDLow()].GetTypeReports(CLIMB_HACK_REPORT), m_Players[player->GetGUIDLow()].GetCreationTime());
}

uint32 AnticheatMgr::GetTotalReports(uint32 lowGUID)
{
	return m_Players[lowGUID].GetTotalReports();
}

float AnticheatMgr::GetAverage(uint32 lowGUID)
{
	return m_Players[lowGUID].GetAverage();
}

uint32 AnticheatMgr::GetTypeReports(uint32 lowGUID, uint8 type)
{
	return m_Players[lowGUID].GetTypeReports(type);
}

bool AnticheatMgr::MustCheckTempReports(uint8 type)
{
	if (type == JUMP_HACK_REPORT)
		return false;

	return true;
}

void AnticheatMgr::BuildReport(Player* player, uint8 reportType)
{
	uint32 key = player->GetGUIDLow();

	if (MustCheckTempReports(reportType))
	{
		uint32 actualTime = getMSTime();

		if (!m_Players[key].GetTempReportsTimer(reportType))
			m_Players[key].SetTempReportsTimer(actualTime, reportType);

		if (getMSTimeDiff(m_Players[key].GetTempReportsTimer(reportType), actualTime) < 3000)
		{
			m_Players[key].SetTempReports(m_Players[key].GetTempReports(reportType) + 1, reportType);

			if (m_Players[key].GetTempReports(reportType) < 3)
				return;
		}
		else
		{
			m_Players[key].SetTempReportsTimer(actualTime, reportType);
			m_Players[key].SetTempReports(1, reportType);
			return;
		}
	}

	// generating creationTime for average calculation
	if (!m_Players[key].GetTotalReports())
		m_Players[key].SetCreationTime(getMSTime());

	// increasing total_reports
	m_Players[key].SetTotalReports(m_Players[key].GetTotalReports() + 1);
	// increasing specific cheat report
	m_Players[key].SetTypeReports(reportType, m_Players[key].GetTypeReports(reportType) + 1);

	// diff time for average calculation
	uint32 diffTime = getMSTimeDiff(m_Players[key].GetCreationTime(), getMSTime()) / IN_MILLISECONDS;

	if (diffTime > 0)
	{
		// Average == Reports per second
		float average = float(m_Players[key].GetTotalReports()) / float(diffTime);
		m_Players[key].SetAverage(average);
	}

	if ((uint32)sConfigMgr->GetIntDefault("Anticheat.MaxReportsForDailyReport", 70) < m_Players[key].GetTotalReports())
	{
		if (!m_Players[key].GetDailyReportState())
		{
			CharacterDatabase.PExecute("REPLACE INTO daily_players_reports (guid,average,total_reports,speed_reports,fly_reports,jump_reports,waterwalk_reports,teleportplane_reports,climb_reports,creation_time) VALUES (%u,%f,%u,%u,%u,%u,%u,%u,%u,%u);", player->GetGUIDLow(), m_Players[player->GetGUIDLow()].GetAverage(), m_Players[player->GetGUIDLow()].GetTotalReports(), m_Players[player->GetGUIDLow()].GetTypeReports(SPEED_HACK_REPORT), m_Players[player->GetGUIDLow()].GetTypeReports(FLY_HACK_REPORT), m_Players[player->GetGUIDLow()].GetTypeReports(JUMP_HACK_REPORT), m_Players[player->GetGUIDLow()].GetTypeReports(WALK_WATER_HACK_REPORT), m_Players[player->GetGUIDLow()].GetTypeReports(TELEPORT_PLANE_HACK_REPORT), m_Players[player->GetGUIDLow()].GetTypeReports(CLIMB_HACK_REPORT), m_Players[player->GetGUIDLow()].GetCreationTime());
			m_Players[key].SetDailyReportState(true);
		}
	}

	if (m_Players[key].GetTotalReports() > (uint32)sConfigMgr->GetIntDefault("Anticheat.ReportsForIngameWarnings", 70))
	{
		// display warning at the center of the screen, hacky way?
		std::string str = "";
		str = "|cFFFFFC00[Playername:|cFF00FFFF[|cFF60FF00" + std::string(player->GetName().c_str()) + "|cFF00FFFF] Possible cheater!";
		WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
		data << str;
		sWorld->SendGlobalGMMessage(&data);
	}
}

void AnticheatMgr::AnticheatGlobalCommand(ChatHandler* handler)
{
	// MySQL will sort all for us, anyway this is not the best way we must only save the anticheat data not whole player's data!.
	ObjectAccessor::SaveAllPlayers();

	QueryResult resultDB = CharacterDatabase.Query("SELECT guid,average,total_reports FROM players_reports_status WHERE total_reports != 0 ORDER BY average ASC LIMIT 3;");
	if (!resultDB)
	{
		handler->PSendSysMessage("No players found.");
		return;
	}
	else
	{
		handler->SendSysMessage("=============================");
		handler->PSendSysMessage("Players with the lowest averages:");
		do
		{
			Field *fieldsDB = resultDB->Fetch();

			uint32 guid = fieldsDB[0].GetUInt32();
			float average = fieldsDB[1].GetFloat();
			uint32 total_reports = fieldsDB[2].GetUInt32();

			if (Player* player = sObjectMgr->GetPlayerByLowGUID(guid))
				handler->PSendSysMessage("Player: %s Average: %f Total Reports: %u", player->GetName().c_str(), average, total_reports);

		} while (resultDB->NextRow());
	}

	resultDB = CharacterDatabase.Query("SELECT guid,average,total_reports FROM players_reports_status WHERE total_reports != 0 ORDER BY total_reports DESC LIMIT 3;");

	// this should never happen
	if (!resultDB)
	{
		handler->PSendSysMessage("No players found.");
		return;
	}
	else
	{
		handler->PSendSysMessage("=============================");
		handler->PSendSysMessage("Players with the more reports:");
		do
		{
			Field *fieldsDB = resultDB->Fetch();

			uint32 guid = fieldsDB[0].GetUInt32();
			float average = fieldsDB[1].GetFloat();
			uint32 total_reports = fieldsDB[2].GetUInt32();

			if (Player* player = sObjectMgr->GetPlayerByLowGUID(guid))
				handler->PSendSysMessage("Player: %s Total Reports: %u Average: %f", player->GetName().c_str(), total_reports, average);

		} while (resultDB->NextRow());
	}
}

void AnticheatMgr::AnticheatDeleteCommand(uint32 guid)
{
	if (!guid)
	{
		for (AnticheatPlayersDataMap::iterator it = m_Players.begin(); it != m_Players.end(); ++it)
		{
			(*it).second.SetTotalReports(0);
			(*it).second.SetAverage(0);
			(*it).second.SetCreationTime(0);
			for (uint8 i = 0; i < MAX_REPORT_TYPES; i++)
			{
				(*it).second.SetTempReports(0, i);
				(*it).second.SetTempReportsTimer(0, i);
				(*it).second.SetTypeReports(i, 0);
			}
		}
		CharacterDatabase.PExecute("DELETE FROM players_reports_status;");
	}
	else
	{
		m_Players[guid].SetTotalReports(0);
		m_Players[guid].SetAverage(0);
		m_Players[guid].SetCreationTime(0);
		for (uint8 i = 0; i < MAX_REPORT_TYPES; i++)
		{
			m_Players[guid].SetTempReports(0, i);
			m_Players[guid].SetTempReportsTimer(0, i);
			m_Players[guid].SetTypeReports(i, 0);
		}
		CharacterDatabase.PExecute("DELETE FROM players_reports_status WHERE guid=%u;", guid);
	}
}

void AnticheatMgr::ResetDailyReportStates()
{
	for (AnticheatPlayersDataMap::iterator it = m_Players.begin(); it != m_Players.end(); ++it)
		m_Players[(*it).first].SetDailyReportState(false);
}
