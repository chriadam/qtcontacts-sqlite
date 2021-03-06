/*
 * Copyright (C) 2013 Jolla Ltd. <andrew.den.exter@jollamobile.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#include "contactsdatabase.h"
#include "contactsengine.h"
#include "trace_p.h"

#include <QDesktopServices>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

#include <QtDebug>

static const char *setupEncoding =
        "\n PRAGMA encoding = \"UTF-16\";";

static const char *setupTempStore =
        "\n PRAGMA temp_store = MEMORY;";

static const char *setupJournal =
        "\n PRAGMA journal_mode = WAL;";

static const char *setupSynchronous =
        "\n PRAGMA synchronous = FULL;";

static const char *createContactsTable =
        "\n CREATE TABLE Contacts ("
        "\n contactId INTEGER PRIMARY KEY ASC AUTOINCREMENT,"
        "\n displayLabel TEXT,"
        "\n firstName TEXT,"
        "\n lowerFirstName TEXT,"
        "\n lastName TEXT,"
        "\n lowerLastName TEXT,"
        "\n middleName TEXT,"
        "\n prefix TEXT,"
        "\n suffix TEXT,"
        "\n customLabel TEXT,"
        "\n syncTarget TEXT NOT NULL,"
        "\n created DATETIME,"
        "\n modified DATETIME,"
        "\n gender TEXT,"
        "\n isFavorite BOOL,"
        "\n hasPhoneNumber BOOL DEFAULT 0,"
        "\n hasEmailAddress BOOL DEFAULT 0,"
        "\n hasOnlineAccount BOOL DEFAULT 0,"
        "\n isOnline BOOL DEFAULT 0,"
        "\n isDeactivated BOOL DEFAULT 0,"
        "\n isIncidental BOOL DEFAULT 0,"
        "\n type INTEGER DEFAULT 0);"; // QContactType::TypeContact

static const char *createAddressesTable =
        "\n CREATE TABLE Addresses ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY ASC,"
        "\n street TEXT,"
        "\n postOfficeBox TEXT,"
        "\n region TEXT,"
        "\n locality TEXT,"
        "\n postCode TEXT,"
        "\n country TEXT,"
        "\n subTypes TEXT);";

static const char *createAnniversariesTable =
        "\n CREATE TABLE Anniversaries ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n originalDateTime DATETIME,"
        "\n calendarId TEXT,"
        "\n subType TEXT);";

static const char *createAvatarsTable =
        "\n CREATE TABLE Avatars ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n imageUrl TEXT,"
        "\n videoUrl TEXT,"
        "\n avatarMetadata TEXT);"; // arbitrary metadata

static const char *createBirthdaysTable =
        "\n CREATE TABLE Birthdays ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n birthday DATETIME,"
        "\n calendarId TEXT);";

static const char *createEmailAddressesTable =
        "\n CREATE TABLE EmailAddresses ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n emailAddress TEXT,"
        "\n lowerEmailAddress TEXT);";

static const char *createFamiliesTable =
        "\n CREATE TABLE Families ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n spouse TEXT,"
        "\n children TEXT);";

static const char *createGeoLocationsTable =
        "\n CREATE TABLE GeoLocations ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n label TEXT,"
        "\n latitude REAL,"
        "\n longitude REAL,"
        "\n accuracy REAL,"
        "\n altitude REAL,"
        "\n altitudeAccuracy REAL,"
        "\n heading REAL,"
        "\n speed REAL,"
        "\n timestamp DATETIME);";

static const char *createGlobalPresencesTable =
        "\n CREATE TABLE GlobalPresences ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n presenceState INTEGER,"
        "\n timestamp DATETIME,"
        "\n nickname TEXT,"
        "\n customMessage TEXT);";

static const char *createGuidsTable =
        "\n CREATE TABLE Guids ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n guid TEXT);";

static const char *createHobbiesTable =
        "\n CREATE TABLE Hobbies ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n hobby TEXT);";

static const char *createNicknamesTable =
        "\n CREATE TABLE Nicknames ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n nickname TEXT,"
        "\n lowerNickname TEXT);";

static const char *createNotesTable =
        "\n CREATE TABLE Notes ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n note TEXT);";

static const char *createOnlineAccountsTable =
        "\n CREATE TABLE OnlineAccounts ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n accountUri TEXT,"
        "\n lowerAccountUri TEXT,"
        "\n protocol TEXT,"
        "\n serviceProvider TEXT,"
        "\n capabilities TEXT,"
        "\n subTypes TEXT,"
        "\n accountPath TEXT,"
        "\n accountIconPath TEXT,"
        "\n enabled BOOL,"
        "\n accountDisplayName TEXT,"
        "\n serviceProviderDisplayName TEXT);";

static const char *createOrganizationsTable =
        "\n CREATE TABLE Organizations ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n name TEXT,"
        "\n role TEXT,"
        "\n title TEXT,"
        "\n location TEXT,"
        "\n department TEXT,"
        "\n logoUrl TEXT);";

static const char *createPhoneNumbersTable =
        "\n CREATE TABLE PhoneNumbers ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n phoneNumber TEXT,"
        "\n subTypes TEXT,"
        "\n normalizedNumber TEXT);";

static const char *createPresencesTable =
        "\n CREATE TABLE Presences ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n presenceState INTEGER,"
        "\n timestamp DATETIME,"
        "\n nickname TEXT,"
        "\n customMessage TEXT);";

static const char *createRingtonesTable =
        "\n CREATE TABLE Ringtones ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n audioRingtone TEXT,"
        "\n videoRingtone TEXT);";

static const char *createTagsTable =
        "\n CREATE TABLE Tags ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n tag TEXT);";

static const char *createUrlsTable =
        "\n CREATE TABLE Urls ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n url TEXT,"
        "\n subTypes TEXT);";

static const char *createOriginMetadataTable =
        "\n CREATE TABLE OriginMetadata ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n id TEXT,"
        "\n groupId TEXT,"
        "\n enabled BOOL);";

static const char *createExtendedDetailsTable =
        "\n CREATE TABLE ExtendedDetails ("
        "\n detailId INTEGER PRIMARY KEY ASC REFERENCES Details (detailId),"
        "\n contactId INTEGER KEY,"
        "\n name TEXT,"
        "\n data BLOB);";

static const char *createDetailsTable =
        "\n CREATE TABLE Details ("
        "\n detailId INTEGER PRIMARY KEY ASC AUTOINCREMENT,"
        "\n contactId INTEGER REFERENCES Contacts (contactId),"
        "\n detail TEXT,"
        "\n detailUri TEXT UNIQUE,"
        "\n linkedDetailUris TEXT,"
        "\n contexts TEXT,"
        "\n accessConstraints INTEGER,"
        "\n provenance TEXT,"
        "\n modifiable BOOL,"
        "\n nonexportable BOOL);";

static const char *createDetailsRemoveIndex =
        "\n CREATE INDEX DetailsRemoveIndex ON Details(contactId, detail);";

static const char *createAddressesDetailsContactIdIndex =
        "\n CREATE INDEX AddressesDetailsContactIdIndex ON Addresses(contactId);";
static const char *createAnniversariesDetailsContactIdIndex =
        "\n CREATE INDEX AnniversariesDetailsContactIdIndex ON Anniversaries(contactId);";
static const char *createAvatarsDetailsContactIdIndex =
        "\n CREATE INDEX AvatarsDetailsContactIdIndex ON Avatars(contactId);";
static const char *createBirthdaysDetailsContactIdIndex =
        "\n CREATE INDEX BirthdaysDetailsContactIdIndex ON Birthdays(contactId);";
static const char *createEmailAddressesDetailsContactIdIndex =
        "\n CREATE INDEX EmailAddressesDetailsContactIdIndex ON EmailAddresses(contactId);";
static const char *createFamiliesDetailsContactIdIndex =
        "\n CREATE INDEX FamiliesDetailsContactIdIndex ON Families(contactId);";
static const char *createGeoLocationsDetailsContactIdIndex =
        "\n CREATE INDEX GeoLocationsDetailsContactIdIndex ON GeoLocations(contactId);";
static const char *createGlobalPresencesDetailsContactIdIndex =
        "\n CREATE INDEX GlobalPresencesDetailsContactIdIndex ON GlobalPresences(contactId);";
static const char *createGuidsDetailsContactIdIndex =
        "\n CREATE INDEX GuidsDetailsContactIdIndex ON Guids(contactId);";
static const char *createHobbiesDetailsContactIdIndex =
        "\n CREATE INDEX HobbiesDetailsContactIdIndex ON Hobbies(contactId);";
static const char *createNicknamesDetailsContactIdIndex =
        "\n CREATE INDEX NicknamesDetailsContactIdIndex ON Nicknames(contactId);";
static const char *createNotesDetailsContactIdIndex =
        "\n CREATE INDEX NotesDetailsContactIdIndex ON Notes(contactId);";
static const char *createOnlineAccountsDetailsContactIdIndex =
        "\n CREATE INDEX OnlineAccountsDetailsContactIdIndex ON OnlineAccounts(contactId);";
static const char *createOrganizationsDetailsContactIdIndex =
        "\n CREATE INDEX OrganizationsDetailsContactIdIndex ON Organizations(contactId);";
static const char *createPhoneNumbersDetailsContactIdIndex =
        "\n CREATE INDEX PhoneNumbersDetailsContactIdIndex ON PhoneNumbers(contactId);";
static const char *createPresencesDetailsContactIdIndex =
        "\n CREATE INDEX PresencesDetailsContactIdIndex ON Presences(contactId);";
static const char *createRingtonesDetailsContactIdIndex =
        "\n CREATE INDEX RingtonesDetailsContactIdIndex ON Ringtones(contactId);";
static const char *createTagsDetailsContactIdIndex =
        "\n CREATE INDEX TagsDetailsContactIdIndex ON Tags(contactId);";
static const char *createUrlsDetailsContactIdIndex =
        "\n CREATE INDEX UrlsDetailsContactIdIndex ON Urls(contactId);";
static const char *createOriginMetadataDetailsContactIdIndex =
        "\n CREATE INDEX OriginMetadataDetailsContactIdIndex ON OriginMetadata(contactId);";
static const char *createExtendedDetailsContactIdIndex =
        "\n CREATE INDEX ExtendedDetailsContactIdIndex ON ExtendedDetails(contactId);";

static const char *createIdentitiesTable =
        "\n CREATE Table Identities ("
        "\n identity INTEGER PRIMARY KEY,"
        "\n contactId INTEGER KEY);";

static const char *createRelationshipsTable =
        "\n CREATE Table Relationships ("
        "\n firstId INTEGER NOT NULL,"
        "\n secondId INTEGER NOT NULL,"
        "\n type TEXT,"
        "\n PRIMARY KEY (firstId, secondId, type));";

static const char *createDeletedContactsTable =
        "\n CREATE TABLE DeletedContacts ("
        "\n contactId INTEGER PRIMARY KEY,"
        "\n syncTarget TEXT,"
        "\n deleted DATETIME);";

static const char *createOOBTable =
        "\n CREATE TABLE OOB ("
        "\n name TEXT PRIMARY KEY,"
        "\n value BLOB,"
        "\n compressed INTEGER DEFAULT 0);";

static const char *createRemoveTrigger =
        "\n CREATE TRIGGER RemoveContactDetails"
        "\n BEFORE DELETE"
        "\n ON Contacts"
        "\n BEGIN"
        "\n  INSERT INTO DeletedContacts (contactId, syncTarget, deleted) VALUES (old.contactId, old.syncTarget, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'));"
        "\n  DELETE FROM Addresses WHERE contactId = old.contactId;"
        "\n  DELETE FROM Anniversaries WHERE contactId = old.contactId;"
        "\n  DELETE FROM Avatars WHERE contactId = old.contactId;"
        "\n  DELETE FROM Birthdays WHERE contactId = old.contactId;"
        "\n  DELETE FROM EmailAddresses WHERE contactId = old.contactId;"
        "\n  DELETE FROM Families WHERE contactId = old.contactId;"
        "\n  DELETE FROM GeoLocations WHERE contactId = old.contactId;"
        "\n  DELETE FROM GlobalPresences WHERE contactId = old.contactId;"
        "\n  DELETE FROM Guids WHERE contactId = old.contactId;"
        "\n  DELETE FROM Hobbies WHERE contactId = old.contactId;"
        "\n  DELETE FROM Nicknames WHERE contactId = old.contactId;"
        "\n  DELETE FROM Notes WHERE contactId = old.contactId;"
        "\n  DELETE FROM OnlineAccounts WHERE contactId = old.contactId;"
        "\n  DELETE FROM Organizations WHERE contactId = old.contactId;"
        "\n  DELETE FROM PhoneNumbers WHERE contactId = old.contactId;"
        "\n  DELETE FROM Presences WHERE contactId = old.contactId;"
        "\n  DELETE FROM Ringtones WHERE contactId = old.contactId;"
        "\n  DELETE FROM Tags WHERE contactId = old.contactId;"
        "\n  DELETE FROM Urls WHERE contactId = old.contactId;"
        "\n  DELETE FROM OriginMetadata WHERE contactId = old.contactId;"
        "\n  DELETE FROM ExtendedDetails WHERE contactId = old.contactId;"
        "\n  DELETE FROM Details WHERE contactId = old.contactId;"
        "\n  DELETE FROM Identities WHERE contactId = old.contactId;"
        "\n  DELETE FROM Relationships WHERE firstId = old.contactId OR secondId = old.contactId;"
        "\n END;";

static const char *createLocalSelfContact =
        "\n INSERT INTO Contacts ("
        "\n contactId,"
        "\n displayLabel,"
        "\n firstName,"
        "\n lowerFirstName,"
        "\n lastName,"
        "\n lowerLastName,"
        "\n middleName,"
        "\n prefix,"
        "\n suffix,"
        "\n customLabel,"
        "\n syncTarget,"
        "\n created,"
        "\n modified,"
        "\n gender,"
        "\n isFavorite)"
        "\n VALUES ("
        "\n 1,"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n 'local',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n 0);";
static const char *createAggregateSelfContact =
        "\n INSERT INTO Contacts ("
        "\n contactId,"
        "\n displayLabel,"
        "\n firstName,"
        "\n lowerFirstName,"
        "\n lastName,"
        "\n lowerLastName,"
        "\n middleName,"
        "\n prefix,"
        "\n suffix,"
        "\n customLabel,"
        "\n syncTarget,"
        "\n created,"
        "\n modified,"
        "\n gender,"
        "\n isFavorite)"
        "\n VALUES ("
        "\n 2,"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n 'aggregate',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n 0);";
static const char *createSelfContactRelationship =
        "\n INSERT INTO Relationships (firstId, secondId, type) VALUES (2, 1, 'Aggregates');";

static const char *createSelfContact =
        "\n INSERT INTO Contacts ("
        "\n contactId,"
        "\n displayLabel,"
        "\n firstName,"
        "\n lowerFirstName,"
        "\n lastName,"
        "\n lowerLastName,"
        "\n middleName,"
        "\n prefix,"
        "\n suffix,"
        "\n customLabel,"
        "\n syncTarget,"
        "\n created,"
        "\n modified,"
        "\n gender,"
        "\n isFavorite)"
        "\n VALUES ("
        "\n 2,"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n 'local',"
        "\n '',"
        "\n '',"
        "\n '',"
        "\n 0);";

static const char *createContactsSyncTargetIndex =
        "\n CREATE INDEX ContactsSyncTargetIndex ON Contacts(syncTarget);";

static const char *createContactsFirstNameIndex =
        "\n CREATE INDEX ContactsFirstNameIndex ON Contacts(lowerFirstName);";

static const char *createContactsLastNameIndex =
        "\n CREATE INDEX ContactsLastNameIndex ON Contacts(lowerLastName);";

static const char *createContactsModifiedIndex =
        "\n CREATE INDEX ContactsModifiedIndex ON Contacts(modified);";

static const char *createContactsIsFavoriteIndex =
        "\n CREATE INDEX ContactsIsFavoriteIndex ON Contacts(isFavorite);";

static const char *createContactsHasPhoneNumberIndex =
        "\n CREATE INDEX ContactsHasPhoneNumberIndex ON Contacts(hasPhoneNumber);";

static const char *createContactsHasEmailAddressIndex =
        "\n CREATE INDEX ContactsHasEmailAddressIndex ON Contacts(hasEmailAddress);";

static const char *createContactsHasOnlineAccountIndex =
        "\n CREATE INDEX ContactsHasOnlineAccountIndex ON Contacts(hasOnlineAccount);";

static const char *createContactsIsOnlineIndex =
        "\n CREATE INDEX ContactsIsOnlineIndex ON Contacts(isOnline);";

static const char *createContactsIsDeactivatedIndex =
        "\n CREATE INDEX ContactsIsDeactivatedIndex ON Contacts(isDeactivated);";

static const char *createContactsTypeIndex =
        "\n CREATE INDEX ContactsTypeIndex ON Contacts(type);";

static const char *createRelationshipsFirstIdIndex =
        "\n CREATE INDEX RelationshipsFirstIdIndex ON Relationships(firstId);";

static const char *createRelationshipsSecondIdIndex =
        "\n CREATE INDEX RelationshipsSecondIdIndex ON Relationships(secondId);";

static const char *createDeletedContactsDeletedIndex =
        "\n CREATE INDEX DeletedContactsDeletedIndex ON DeletedContacts(deleted);";

static const char *createPhoneNumbersIndex =
        "\n CREATE INDEX PhoneNumbersIndex ON PhoneNumbers(normalizedNumber);";

static const char *createEmailAddressesIndex =
        "\n CREATE INDEX EmailAddressesIndex ON EmailAddresses(lowerEmailAddress);";

static const char *createOnlineAccountsIndex =
        "\n CREATE INDEX OnlineAccountsIndex ON OnlineAccounts(lowerAccountUri);";

static const char *createNicknamesIndex =
        "\n CREATE INDEX NicknamesIndex ON Nicknames(lowerNickname);";

static const char *createOriginMetadataIdIndex =
        "\n CREATE INDEX OriginMetadataIdIndex ON OriginMetadata(id);";

static const char *createOriginMetadataGroupIdIndex =
        "\n CREATE INDEX OriginMetadataGroupIdIndex ON OriginMetadata(groupId);";

static const char *createStatements[] =
{
    createContactsTable,
    createAddressesTable,
    createAnniversariesTable,
    createAvatarsTable,
    createBirthdaysTable,
    createEmailAddressesTable,
    createFamiliesTable,
    createGeoLocationsTable,
    createGlobalPresencesTable,
    createGuidsTable,
    createHobbiesTable,
    createNicknamesTable,
    createNotesTable,
    createOnlineAccountsTable,
    createOrganizationsTable,
    createPhoneNumbersTable,
    createPresencesTable,
    createRingtonesTable,
    createTagsTable,
    createUrlsTable,
    createOriginMetadataTable,
    createExtendedDetailsTable,
    createDetailsTable,
    createDetailsRemoveIndex,
    createAddressesDetailsContactIdIndex,
    createAnniversariesDetailsContactIdIndex,
    createAvatarsDetailsContactIdIndex,
    createBirthdaysDetailsContactIdIndex,
    createEmailAddressesDetailsContactIdIndex,
    createGlobalPresencesDetailsContactIdIndex,
    createGuidsDetailsContactIdIndex,
    createHobbiesDetailsContactIdIndex,
    createNicknamesDetailsContactIdIndex,
    createNotesDetailsContactIdIndex,
    createOnlineAccountsDetailsContactIdIndex,
    createOrganizationsDetailsContactIdIndex,
    createPhoneNumbersDetailsContactIdIndex,
    createPresencesDetailsContactIdIndex,
    createRingtonesDetailsContactIdIndex,
    createTagsDetailsContactIdIndex,
    createUrlsDetailsContactIdIndex,
    createOriginMetadataDetailsContactIdIndex,
    createExtendedDetailsContactIdIndex,
    createIdentitiesTable,
    createRelationshipsTable,
    createDeletedContactsTable,
    createOOBTable,
    createRemoveTrigger,
    createContactsSyncTargetIndex,
    createContactsFirstNameIndex,
    createContactsLastNameIndex,
    createRelationshipsFirstIdIndex,
    createRelationshipsSecondIdIndex,
    createDeletedContactsDeletedIndex,
    createPhoneNumbersIndex,
    createEmailAddressesIndex,
    createOnlineAccountsIndex,
    createNicknamesIndex,
    createOriginMetadataIdIndex,
    createOriginMetadataGroupIdIndex,
    createContactsModifiedIndex,
    createContactsIsFavoriteIndex,
    createContactsHasPhoneNumberIndex,
    createContactsHasEmailAddressIndex,
    createContactsHasOnlineAccountIndex,
    createContactsIsOnlineIndex,
    createContactsIsDeactivatedIndex,
    createContactsTypeIndex,
};

// Upgrade statement indexed by old version
static const char *upgradeVersion0[] = {
    createContactsModifiedIndex,
    createContactsIsFavoriteIndex,
    createContactsHasPhoneNumberIndex,
    createContactsHasEmailAddressIndex,
    createContactsHasOnlineAccountIndex,
    createContactsIsOnlineIndex,
    "PRAGMA user_version=1",
    0 // NULL-terminated
};
static const char *upgradeVersion1[] = {
    createDeletedContactsTable,
    createDeletedContactsDeletedIndex,
    "DROP TRIGGER RemoveContactDetails",
    createRemoveTrigger,
    "PRAGMA user_version=2",
    0 // NULL-terminated
};
static const char *upgradeVersion2[] = {
    "ALTER TABLE Contacts ADD COLUMN isDeactivated BOOL DEFAULT 0",
    createContactsIsDeactivatedIndex,
    "PRAGMA user_version=3",
    0 // NULL-terminated
};
static const char *upgradeVersion3[] = {
    "ALTER TABLE Contacts ADD COLUMN isIncidental BOOL DEFAULT 0",
    "PRAGMA user_version=4",
    0 // NULL-terminated
};
static const char *upgradeVersion4[] = {
    // We can't create this in final form anymore, since we're modifying it in version 8->9
    //createOOBTable,
    "CREATE TABLE OOB ("
        "name TEXT PRIMARY KEY,"
        "value BLOB)",
    "PRAGMA user_version=5",
    0 // NULL-terminated
};
static const char *upgradeVersion5[] = {
    // Create the isDeactivated index, if it was previously missed
    "CREATE INDEX IF NOT EXISTS ContactsIsDeactivatedIndex ON Contacts(isDeactivated)",
    "ALTER TABLE Contacts ADD COLUMN type INTEGER DEFAULT 0",
    createContactsTypeIndex,
    "PRAGMA user_version=6",
    0 // NULL-terminated
};
static const char *upgradeVersion6[] = {
    "ALTER TABLE Details ADD COLUMN nonexportable BOOL DEFAULT 0",
    "PRAGMA user_version=7",
    0 // NULL-terminated
};
static const char *upgradeVersion7[] = {
    "PRAGMA user_version=8",
    0 // NULL-terminated
};
static const char *upgradeVersion8[] = {
    // Alter the OOB table; this alteration requires that the earlier upgrade
    // creates the obsolete form of the table rather thna the current one
    "ALTER TABLE OOB ADD COLUMN compressed INTEGER DEFAULT 0",
    "PRAGMA user_version=9",
    0 // NULL-terminated
};
static const char *upgradeVersion9[] = {
    "DROP INDEX DetailsJoinIndex",
    // Don't recreate the index since it doesn't exist in versions after 10:
    //createDetailsJoinIndex,
    "PRAGMA user_version=10",
    0 // NULL-terminated
};
static const char *upgradeVersion10[] = {
    // Drop the remove trigger
    "DROP TRIGGER RemoveContactDetails",
    // Preserve the existing state of the Details table
    "ALTER TABLE Details RENAME TO OldDetails",
    // Create an index to map new version of detail rows to the old ones
    "CREATE TEMP TABLE DetailsIndexing("
        "detailId INTEGER PRIMARY KEY ASC AUTOINCREMENT,"
        "oldDetailId INTEGER,"
        "contactId INTEGER,"
        "detail TEXT,"
        "syncTarget TEXT,"
        "provenance TEXT)",
    "INSERT INTO DetailsIndexing(oldDetailId, contactId, detail, syncTarget, provenance) "
        "SELECT OD.detailId, OD.contactId, OD.detail, Contacts.syncTarget, CASE WHEN Contacts.syncTarget = 'aggregate' THEN OD.provenance ELSE '' END "
        "FROM OldDetails AS OD "
        "JOIN Contacts ON Contacts.contactId = OD.contactId",
    // Index the indexing table by the detail ID and type name used to select from it
    "CREATE INDEX DetailsIndexingOldDetailIdIndex ON DetailsIndexing(oldDetailId)",
    "CREATE INDEX DetailsIndexingDetailIndex ON DetailsIndexing(detail)",
    // Find the new detail ID for existing provenance ID values
    "CREATE TEMP TABLE ProvenanceIndexing("
        "detailId INTEGER PRIMARY KEY,"
        "detail TEXT,"
        "provenance TEXT,"
        "provenanceContactId TEXT,"
        "provenanceDetailId TEXT,"
        "provenanceSyncTarget TEXT,"
        "newProvenanceDetailId TEXT)",
    "INSERT INTO ProvenanceIndexing(detailId, detail, provenance) "
        "SELECT detailId, detail, provenance "
        "FROM DetailsIndexing "
        "WHERE provenance != ''",
    // Calculate the new equivalent form for the existing 'provenance' values
    "UPDATE ProvenanceIndexing SET "
        "provenanceContactId = substr(provenance, 0, instr(provenance, ':')),"
        "provenance = substr(provenance, instr(provenance, ':') + 1)",
    "UPDATE ProvenanceIndexing SET "
        "provenanceDetailId = substr(provenance, 0, instr(provenance, ':')),"
        "provenanceSyncTarget = substr(provenance, instr(provenance, ':') + 1),"
        "provenance = ''",
    "REPLACE INTO ProvenanceIndexing (detailId, provenance) "
        "SELECT PI.detailId, PI.provenanceContactId || ':' || DI.detailId || ':' || PI.provenanceSyncTarget "
        "FROM ProvenanceIndexing AS PI "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = PI.provenanceDetailId AND DI.detail = PI.detail",
    // Update the provenance values in the DetailsIndexing table with the updated values
    "REPLACE INTO DetailsIndexing (detailId, oldDetailId, contactId, detail, syncTarget, provenance) "
        "SELECT PI.detailId, DI.oldDetailId, DI.contactId, DI.detail, DI.syncTarget, PI.provenance "
        "FROM ProvenanceIndexing PI "
        "JOIN DetailsIndexing DI ON DI.detailId = PI.detailId",
    "DROP TABLE ProvenanceIndexing",
    // Re-create and populate the Details table from the old version
    createDetailsTable,
    "INSERT INTO Details("
            "detailId,"
            "contactId,"
            "detail,"
            "detailUri,"
            "linkedDetailUris,"
            "contexts,"
            "accessConstraints,"
            "provenance,"
            "modifiable,"
            "nonexportable) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.detail,"
            "OD.detailUri,"
            "OD.linkedDetailUris,"
            "OD.contexts,"
            "OD.accessConstraints,"
            "DI.provenance,"
            "OD.modifiable,"
            "OD.nonexportable "
        "FROM DetailsIndexing AS DI "
        "JOIN OldDetails AS OD ON OD.detailId = DI.oldDetailId AND OD.detail = DI.detail",
    "DROP INDEX IF EXISTS DetailsJoinIndex",
    "DROP INDEX DetailsRemoveIndex",
    "DROP TABLE OldDetails",
    // Drop all indexes for tables we are rebuilding
    "DROP INDEX createAddressesDetailsContactIdIndex",
    "DROP INDEX createAnniversariesDetailsContactIdIndex",
    "DROP INDEX createAvatarsDetailsContactIdIndex",
    "DROP INDEX createBirthdaysDetailsContactIdIndex",
    "DROP INDEX createEmailAddressesDetailsContactIdIndex",
    "DROP INDEX createGlobalPresencesDetailsContactIdIndex",
    "DROP INDEX createGuidsDetailsContactIdIndex",
    "DROP INDEX createHobbiesDetailsContactIdIndex",
    "DROP INDEX createNicknamesDetailsContactIdIndex",
    "DROP INDEX createNotesDetailsContactIdIndex",
    "DROP INDEX createOnlineAccountsDetailsContactIdIndex",
    "DROP INDEX createOrganizationsDetailsContactIdIndex",
    "DROP INDEX createPhoneNumbersDetailsContactIdIndex",
    "DROP INDEX createPresencesDetailsContactIdIndex",
    "DROP INDEX createRingtonesDetailsContactIdIndex",
    "DROP INDEX createTagsDetailsContactIdIndex",
    "DROP INDEX createUrlsDetailsContactIdIndex",
    "DROP INDEX createTpMetadataDetailsContactIdIndex",
    "DROP INDEX createExtendedDetailsContactIdIndex",
    "DROP INDEX PhoneNumbersIndex",
    "DROP INDEX EmailAddressesIndex",
    "DROP INDEX OnlineAccountsIndex",
    "DROP INDEX NicknamesIndex",
    "DROP INDEX TpMetadataTelepathyIdIndex",
    "DROP INDEX TpMetadataAccountIdIndex",
    // Migrate the Addresses table to the new form
    "ALTER TABLE Addresses RENAME TO OldAddresses",
    createAddressesTable,
    "INSERT INTO Addresses("
            "detailId,"
            "contactId,"
            "street,"
            "postOfficeBox,"
            "region,"
            "locality,"
            "postCode,"
            "country,"
            "subTypes) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.street,"
            "OD.postOfficeBox,"
            "OD.region,"
            "OD.locality,"
            "OD.postCode,"
            "OD.country,"
            "OD.subTypes "
        "FROM OldAddresses AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'Address'",
    "DROP TABLE OldAddresses",
    // Migrate the Anniversaries table to the new form
    "ALTER TABLE Anniversaries RENAME TO OldAnniversaries",
    createAnniversariesTable,
    "INSERT INTO Anniversaries("
            "detailId,"
            "contactId,"
            "originalDateTime,"
            "calendarId,"
            "subType) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.originalDateTime,"
            "OD.calendarId,"
            "OD.subType "
        "FROM OldAnniversaries AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'Anniversary'",
    "DROP TABLE OldAnniversaries",
    // Migrate the Avatars table to the new form
    "ALTER TABLE Avatars RENAME TO OldAvatars",
    createAvatarsTable,
    "INSERT INTO Avatars("
            "detailId,"
            "contactId,"
            "imageUrl,"
            "videoUrl,"
            "avatarMetadata) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.imageUrl,"
            "OD.videoUrl,"
            "OD.avatarMetadata "
        "FROM OldAvatars AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'Avatar'",
    "DROP TABLE OldAvatars",
    // Migrate the Birthdays table to the new form
    "ALTER TABLE Birthdays RENAME TO OldBirthdays",
    createBirthdaysTable,
    "INSERT INTO Birthdays("
            "detailId,"
            "contactId,"
            "birthday,"
            "calendarId) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.birthday,"
            "OD.calendarId "
        "FROM OldBirthdays AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'Birthday'",
    "DROP TABLE OldBirthdays",
    // Migrate the EmailAddresses table to the new form
    "ALTER TABLE EmailAddresses RENAME TO OldEmailAddresses",
    createEmailAddressesTable,
    "INSERT INTO EmailAddresses("
            "detailId,"
            "contactId,"
            "emailAddress,"
            "lowerEmailAddress) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.emailAddress,"
            "OD.lowerEmailAddress "
        "FROM OldEmailAddresses AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'EmailAddress'",
    "DROP TABLE OldEmailAddresses",
    // Migrate the GlobalPresences table to the new form
    "ALTER TABLE GlobalPresences RENAME TO OldGlobalPresences",
    createGlobalPresencesTable,
    "INSERT INTO GlobalPresences("
            "detailId,"
            "contactId,"
            "presenceState,"
            "timestamp,"
            "nickname,"
            "customMessage) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.presenceState,"
            "OD.timestamp,"
            "OD.nickname,"
            "OD.customMessage "
        "FROM OldGlobalPresences AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'GlobalPresence'",
    "DROP TABLE OldGlobalPresences",
    // Migrate the Guids table to the new form
    "ALTER TABLE Guids RENAME TO OldGuids",
    createGuidsTable,
    "INSERT INTO Guids("
            "detailId,"
            "contactId,"
            "guid) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.guid "
        "FROM OldGuids AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'Guid'",
    "DROP TABLE OldGuids",
    // Migrate the Hobbies table to the new form
    "ALTER TABLE Hobbies RENAME TO OldHobbies",
    createHobbiesTable,
    "INSERT INTO Hobbies("
            "detailId,"
            "contactId,"
            "hobby) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.hobby "
        "FROM OldHobbies AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'Hobby'",
    "DROP TABLE OldHobbies",
    // Migrate the Nicknames table to the new form
    "ALTER TABLE Nicknames RENAME TO OldNicknames",
    createNicknamesTable,
    "INSERT INTO Nicknames("
            "detailId,"
            "contactId,"
            "nickname,"
            "lowerNickname) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.nickname,"
            "OD.lowerNickname "
        "FROM OldNicknames AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'Nickname'",
    "DROP TABLE OldNicknames",
    // Migrate the Notes table to the new form
    "ALTER TABLE Notes RENAME TO OldNotes",
    createNotesTable,
    "INSERT INTO Notes("
            "detailId,"
            "contactId,"
            "note) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.note "
        "FROM OldNotes AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'Note'",
    "DROP TABLE OldNotes",
    // Migrate the OnlineAccounts table to the new form
    "ALTER TABLE OnlineAccounts RENAME TO OldOnlineAccounts",
    createOnlineAccountsTable,
    "INSERT INTO OnlineAccounts("
            "detailId,"
            "contactId,"
            "accountUri,"
            "lowerAccountUri,"
            "protocol,"
            "serviceProvider,"
            "capabilities,"
            "subTypes,"
            "accountPath,"
            "accountIconPath,"
            "enabled,"
            "accountDisplayName,"
            "serviceProviderDisplayName) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.accountUri,"
            "OD.lowerAccountUri,"
            "OD.protocol,"
            "OD.serviceProvider,"
            "OD.capabilities,"
            "OD.subTypes,"
            "OD.accountPath,"
            "OD.accountIconPath,"
            "OD.enabled,"
            "OD.accountDisplayName,"
            "OD.serviceProviderDisplayName "
        "FROM OldOnlineAccounts AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'OnlineAccount'",
    "DROP TABLE OldOnlineAccounts",
    // Migrate the Organizations table to the new form
    "ALTER TABLE Organizations RENAME TO OldOrganizations",
    createOrganizationsTable,
    "INSERT INTO Organizations("
            "detailId,"
            "contactId,"
            "name,"
            "role,"
            "title,"
            "location,"
            "department,"
            "logoUrl) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.name,"
            "OD.role,"
            "OD.title,"
            "OD.location,"
            "OD.department,"
            "OD.logoUrl "
        "FROM OldOrganizations AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'Organization'",
    "DROP TABLE OldOrganizations",
    // Migrate the PhoneNumbers table to the new form
    "ALTER TABLE PhoneNumbers RENAME TO OldPhoneNumbers",
    createPhoneNumbersTable,
    "INSERT INTO PhoneNumbers("
            "detailId,"
            "contactId,"
            "phoneNumber,"
            "subTypes,"
            "normalizedNumber) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.phoneNumber,"
            "OD.subTypes,"
            "OD.normalizedNumber "
        "FROM OldPhoneNumbers AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'PhoneNumber'",
    "DROP TABLE OldPhoneNumbers",
    // Migrate the Presences table to the new form
    "ALTER TABLE Presences RENAME TO OldPresences",
    createPresencesTable,
    "INSERT INTO Presences("
            "detailId,"
            "contactId,"
            "presenceState,"
            "timestamp,"
            "nickname,"
            "customMessage) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.presenceState,"
            "OD.timestamp,"
            "OD.nickname,"
            "OD.customMessage "
        "FROM OldPresences AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'Presence'",
    "DROP TABLE OldPresences",
    // Migrate the Ringtones table to the new form
    "ALTER TABLE Ringtones RENAME TO OldRingtones",
    createRingtonesTable,
    "INSERT INTO Ringtones("
            "detailId,"
            "contactId,"
            "audioRingtone,"
            "videoRingtone) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.audioRingtone,"
            "OD.videoRingtone "
        "FROM OldRingtones AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'Ringtone'",
    "DROP TABLE OldRingtones",
    // Migrate the Tags table to the new form
    "ALTER TABLE Tags RENAME TO OldTags",
    createTagsTable,
    "INSERT INTO Tags("
            "detailId,"
            "contactId,"
            "tag) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.tag "
        "FROM OldTags AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'Tag'",
    "DROP TABLE OldTags",
    // Migrate the Urls table to the new form
    "ALTER TABLE Urls RENAME TO OldUrls",
    createUrlsTable,
    "INSERT INTO Urls("
            "detailId,"
            "contactId,"
            "url,"
            "subTypes) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.url,"
            "OD.subTypes "
        "FROM OldUrls AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'Url'",
    "DROP TABLE OldUrls",
    // Migrate the TpMetadata table to the new form (and rename it to the correct name)
    createOriginMetadataTable,
    "INSERT INTO OriginMetadata("
            "detailId,"
            "contactId,"
            "id,"
            "groupId,"
            "enabled) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.telepathyId,"
            "OD.accountId,"
            "OD.accountEnabled "
        "FROM TpMetadata AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'OriginMetadata'",
    "DROP TABLE TpMetadata",
    // Migrate the ExtendedDetails table to the new form
    "ALTER TABLE ExtendedDetails RENAME TO OldExtendedDetails",
    createExtendedDetailsTable,
    "INSERT INTO ExtendedDetails("
            "detailId,"
            "contactId,"
            "name,"
            "data) "
        "SELECT "
            "DI.detailId,"
            "OD.contactId,"
            "OD.name,"
            "OD.data "
        "FROM OldExtendedDetails AS OD "
        "JOIN DetailsIndexing AS DI ON DI.oldDetailId = OD.detailId AND DI.detail = 'ExtendedDetail'",
    "DROP TABLE OldExtendedDetails",
    // Drop the indexing table
    "DROP INDEX DetailsIndexingOldDetailIdIndex",
    "DROP INDEX DetailsIndexingDetailIndex",
    "DROP TABLE DetailsIndexing",
    // Rebuild the indexes we dropped
    createDetailsRemoveIndex,
    createAddressesDetailsContactIdIndex,
    createAnniversariesDetailsContactIdIndex,
    createAvatarsDetailsContactIdIndex,
    createBirthdaysDetailsContactIdIndex,
    createEmailAddressesDetailsContactIdIndex,
    createGlobalPresencesDetailsContactIdIndex,
    createGuidsDetailsContactIdIndex,
    createHobbiesDetailsContactIdIndex,
    createNicknamesDetailsContactIdIndex,
    createNotesDetailsContactIdIndex,
    createOnlineAccountsDetailsContactIdIndex,
    createOrganizationsDetailsContactIdIndex,
    createPhoneNumbersDetailsContactIdIndex,
    createPresencesDetailsContactIdIndex,
    createRingtonesDetailsContactIdIndex,
    createTagsDetailsContactIdIndex,
    createUrlsDetailsContactIdIndex,
    createOriginMetadataDetailsContactIdIndex,
    createExtendedDetailsContactIdIndex,
    createPhoneNumbersIndex,
    createEmailAddressesIndex,
    createOnlineAccountsIndex,
    createNicknamesIndex,
    createOriginMetadataIdIndex,
    createOriginMetadataGroupIdIndex,
    // Recreate the remove trigger
    createRemoveTrigger,
    // Finished
    "PRAGMA user_version=11",
    0 // NULL-terminated
};
static const char *upgradeVersion11[] = {
    createFamiliesTable,
    createGeoLocationsTable,
    // Recreate the remove trigger to include these details
    "DROP TRIGGER RemoveContactDetails",
    createRemoveTrigger,
    "PRAGMA user_version=12",
    0 // NULL-terminated
};

typedef bool (*UpgradeFunction)(QSqlDatabase &database);

struct UpdatePhoneNormalization
{
    quint32 detailId;
    QString normalizedNumber;
};
static bool updateNormalizedNumbers(QSqlDatabase &database)
{
    QList<UpdatePhoneNormalization> updates;

    QString statement(QStringLiteral("SELECT detailId, phoneNumber, normalizedNumber FROM PhoneNumbers"));
    QSqlQuery query(database);
    if (!query.exec(statement)) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Query failed: %1\n%2")
                .arg(query.lastError().text())
                .arg(statement));
        return false;
    }
    while (query.next()) {
        const quint32 detailId(query.value(0).value<quint32>());
        const QString number(query.value(1).value<QString>());
        const QString normalized(query.value(2).value<QString>());

        const QString currentNormalization(ContactsEngine::normalizedPhoneNumber(number));
        if (currentNormalization != normalized) {
            UpdatePhoneNormalization data = { detailId, currentNormalization };
            updates.append(data);
        }
    }
    query.finish();

    if (!updates.isEmpty()) {
        query = QSqlQuery(database);
        statement = QStringLiteral("UPDATE PhoneNumbers SET normalizedNumber = :normalizedNumber WHERE detailId = :detailId");
        if (!query.prepare(statement)) {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare data upgrade query: %1\n%2")
                    .arg(query.lastError().text())
                    .arg(statement));
            return false;
        }

        foreach (const UpdatePhoneNormalization &update, updates) {
            query.bindValue(":normalizedNumber", update.normalizedNumber);
            query.bindValue(":detailId", update.detailId);
            if (!query.exec()) {
                QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to upgrade data: %1\n%2")
                        .arg(query.lastError().text())
                        .arg(statement));
                return false;
            }
            query.finish();
        }
    }

    return true;
}

struct UpgradeOperation {
    UpgradeFunction fn;
    const char **statements;
};

static UpgradeOperation upgradeVersions[] = {
    { 0,                        upgradeVersion0 },
    { 0,                        upgradeVersion1 },
    { 0,                        upgradeVersion2 },
    { 0,                        upgradeVersion3 },
    { 0,                        upgradeVersion4 },
    { 0,                        upgradeVersion5 },
    { 0,                        upgradeVersion6 },
    { updateNormalizedNumbers,  upgradeVersion7 },
    { 0,                        upgradeVersion8 },
    { 0,                        upgradeVersion9 },
    { 0,                        upgradeVersion10 },
    { 0,                        upgradeVersion11 },
};

static const int currentSchemaVersion = 12;

static bool execute(QSqlDatabase &database, const QString &statement)
{
    QSqlQuery query(database);
    if (!query.exec(statement)) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Query failed: %1\n%2")
                .arg(query.lastError().text())
                .arg(statement));
        return false;
    } else {
        return true;
    }
}

static bool beginTransaction(QSqlDatabase &database)
{
    // Use immediate lock acquisition; we should already have an IPC lock, so
    // there will be no lock contention with other writing processes
    return execute(database, QString::fromLatin1("BEGIN IMMEDIATE TRANSACTION"));
}

static bool commitTransaction(QSqlDatabase &database)
{
    return execute(database, QString::fromLatin1("COMMIT TRANSACTION"));
}

static bool rollbackTransaction(QSqlDatabase &database)
{
    return execute(database, QString::fromLatin1("ROLLBACK TRANSACTION"));
}

static bool finalizeTransaction(QSqlDatabase &database, bool success)
{
    if (success) {
        return commitTransaction(database);
    }

    rollbackTransaction(database);
    return false;
}

template <typename T> static int lengthOf(T) { return 0; }
template <typename T, int N> static int lengthOf(const T(&)[N]) { return N; }

static bool executeUpgradeStatements(QSqlDatabase &database)
{
    // Check that the defined schema matches the array of upgrade scripts
    if (currentSchemaVersion != lengthOf(upgradeVersions)) {
        qWarning() << "Invalid schema version:" << currentSchemaVersion;
        return false;
    }

    QSqlQuery versionQuery(database);
    versionQuery.prepare("PRAGMA user_version");
    if (!versionQuery.exec() || !versionQuery.next()) {
        qWarning() << "User version query failed:" << versionQuery.lastError();
        return false;
    }

    int schemaVersion = versionQuery.value(0).toInt();
    versionQuery.finish();

    while (schemaVersion < currentSchemaVersion) {
        qWarning() << "Upgrading contacts database from schema version" << schemaVersion;

        if (upgradeVersions[schemaVersion].fn) {
            if (!(*upgradeVersions[schemaVersion].fn)(database)) {
                qWarning() << "Unable to update data for schema version" << schemaVersion;
                return false;
            }
        }
        if (upgradeVersions[schemaVersion].statements) {
            for (unsigned i = 0; upgradeVersions[schemaVersion].statements[i]; i++) {
                if (!execute(database, QLatin1String(upgradeVersions[schemaVersion].statements[i])))
                    return false;
            }
        }

        if (!versionQuery.exec() || !versionQuery.next()) {
            qWarning() << "User version query failed:" << versionQuery.lastError();
            return false;
        }

        int version = versionQuery.value(0).toInt();
        versionQuery.finish();

        if (version <= schemaVersion) {
            qWarning() << "Contacts database schema upgrade cycle detected - aborting";
            return false;
        } else {
            schemaVersion = version;
            if (schemaVersion == currentSchemaVersion) {
                qWarning() << "Contacts database upgraded to version" << schemaVersion;
            }
        }
    }

    if (schemaVersion > currentSchemaVersion) {
        qWarning() << "Contacts database schema is newer than expected - this may result in failures or corruption";
    }

    return true;
}

static bool checkDatabase(QSqlDatabase &database)
{
    QSqlQuery query(database);
    if (query.exec(QLatin1String("PRAGMA quick_check"))) {
        while (query.next()) {
            const QString result(query.value(0).toString());
            if (result == QLatin1String("ok")) {
                return true;
            }
            qWarning() << "Integrity problem:" << result;
        }
    }

    return false;
}

static bool upgradeDatabase(QSqlDatabase &database)
{
    if (!beginTransaction(database))
        return false;

    bool success = executeUpgradeStatements(database);

    return finalizeTransaction(database, success);
}

static bool configureDatabase(QSqlDatabase &database, QString &localeName)
{
    if (!execute(database, QLatin1String(setupEncoding))
        || !execute(database, QLatin1String(setupTempStore))
        || !execute(database, QLatin1String(setupJournal))
        || !execute(database, QLatin1String(setupSynchronous))) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to configure contacts database: %1")
                .arg(database.lastError().text()));
        return false;
    } else {
        const QString cLocaleName(QString::fromLatin1("C"));
        if (localeName != cLocaleName) {
            // Create a collation for sorting by the current locale
            const QString statement(QString::fromLatin1("SELECT icu_load_collation('%1', 'localeCollation')"));
            if (!execute(database, statement.arg(localeName))) {
                QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to configure collation for locale %1: %2")
                        .arg(localeName).arg(database.lastError().text()));

                // Revert to using C locale for sorting
                localeName = cLocaleName;
            }
        }
    }

    return true;
}

static bool executeCreationStatements(QSqlDatabase &database)
{
    for (int i = 0; i < lengthOf(createStatements); ++i) {
        QSqlQuery query(database);

        if (!query.exec(QLatin1String(createStatements[i]))) {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Database creation failed: %1\n%2")
                    .arg(query.lastError().text())
                    .arg(createStatements[i]));
            return false;
        }
    }

    if (!execute(database, QString::fromLatin1("PRAGMA user_version=%1").arg(currentSchemaVersion))) {
        return false;
    }

    return true;
}

static bool executeSelfContactStatements(QSqlDatabase &database, const bool aggregating)
{
    const char *createStatements[] = {
        createSelfContact,
        0
    };
    const char *aggregatingCreateStatements[] = {
        createLocalSelfContact,
        createAggregateSelfContact,
        createSelfContactRelationship,
        0
    };

    const char **statement = (aggregating ? aggregatingCreateStatements : createStatements);
    for ( ; *statement != 0; ++statement) {
        QSqlQuery query(database);
        if (!query.exec(QString::fromLatin1(*statement))) {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Database creation failed: %1\n%2")
                    .arg(query.lastError().text())
                    .arg(*statement));
            return false;
        }
    }

    return true;
}

static bool prepareDatabase(QSqlDatabase &database, const bool aggregating, QString &localeName)
{
    if (!configureDatabase(database, localeName))
        return false;

    if (!beginTransaction(database))
        return false;

    bool success = executeCreationStatements(database);
    if (success) {
        success = executeSelfContactStatements(database, aggregating);
    }

    return finalizeTransaction(database, success);
}

template<typename ValueContainer>
static void debugFilterExpansion(const QString &description, const QString &query, const ValueContainer &bindings)
{
    static const bool debugFilters = !qgetenv("QTCONTACTS_SQLITE_DEBUG_FILTERS").isEmpty();

    if (debugFilters) {
        qDebug() << description << ContactsDatabase::expandQuery(query, bindings);
    }
}

static void bindValues(QSqlQuery &query, const QVariantList &values)
{
    for (int i = 0; i < values.count(); ++i) {
        query.bindValue(i, values.at(i));
    }
}

static void bindValues(QSqlQuery &query, const QMap<QString, QVariant> &values)
{
    QMap<QString, QVariant>::const_iterator it = values.constBegin(), end = values.constEnd();
    for ( ; it != end; ++it) {
        query.bindValue(it.key(), it.value());
    }
}

static bool countTransientTables(QSqlDatabase &db, const QString &table, int *count)
{
    static const QString sql(QString::fromLatin1("SELECT COUNT(*) FROM sqlite_temp_master WHERE type = 'table' and name LIKE '%1_transient%'"));

    *count = 0;

    QSqlQuery query(db);
    if (!query.prepare(sql.arg(table)) || !ContactsDatabase::execute(query)) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Unable to count transient tables for table: %1").arg(table));
        return false;
    } else while (query.next()) {
        *count = query.value(0).toInt();
    }

    return true;
}

static bool findTransientTables(QSqlDatabase &db, const QString &table, QStringList *tableNames)
{
    static const QString sql(QString::fromLatin1("SELECT name FROM sqlite_temp_master WHERE type = 'table' and name LIKE '%1_transient%'"));

    QSqlQuery query(db);
    if (!query.prepare(sql.arg(table)) || !ContactsDatabase::execute(query)) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Unable to query transient tables for table: %1").arg(table));
        return false;
    } else while (query.next()) {
        tableNames->append(query.value(0).toString());
    }

    return true;
}

static bool dropTransientTables(QSqlDatabase &db, const QString &table)
{
    static const QString dropTableStatement = QString::fromLatin1("DROP TABLE temp.%1");

    QStringList tableNames;
    if (!findTransientTables(db, table, &tableNames))
        return false;

    foreach (const QString tableName, tableNames) {
        QSqlQuery dropTableQuery(db);
        const QString dropStatement(dropTableStatement.arg(tableName));
        if (!dropTableQuery.prepare(dropStatement)) {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare drop transient table query: %1\n%2")
                    .arg(dropTableQuery.lastError().text())
                    .arg(dropStatement));
            return false;
        }
        if (!ContactsDatabase::execute(dropTableQuery)) {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to drop transient temporary table: %1\n%2")
                    .arg(dropTableQuery.lastError().text())
                    .arg(dropStatement));
            return false;
        }
    }

    return true;
}

template<typename ValueContainer>
bool createTemporaryContactIdsTable(QSqlDatabase &db, const QString &table, bool filter, const QVariantList &boundIds, 
                                    const QString &join, const QString &where, const QString &orderBy, const ValueContainer &boundValues, int limit)
{
    static const QString createStatement(QString::fromLatin1("CREATE TABLE IF NOT EXISTS temp.%1 (contactId INTEGER)"));
    static const QString insertFilterStatement(QString::fromLatin1("INSERT INTO temp.%1 (contactId) SELECT Contacts.contactId FROM Contacts %2 %3"));

    // Create the temporary table (if we haven't already).
    QSqlQuery tableQuery(db);
    if (!tableQuery.prepare(createStatement.arg(table))) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare temporary table query: %1\n%2")
                .arg(tableQuery.lastError().text())
                .arg(createStatement));
        return false;
    }
    if (!ContactsDatabase::execute(tableQuery)) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to create temporary table: %1\n%2")
                .arg(tableQuery.lastError().text())
                .arg(createStatement));
        return false;
    }
    tableQuery.finish();

    // insert into the temporary table, all of the ids
    // which will be specified either by id list, or by filter.
    QSqlQuery insertQuery(db);
    if (filter) {
        // specified by filter
        QString insertStatement = insertFilterStatement.arg(table).arg(join).arg(where);
        if (!orderBy.isEmpty()) {
            insertStatement.append(QString::fromLatin1(" ORDER BY ") + orderBy);
        }
        if (limit > 0) {
            insertStatement.append(QString::fromLatin1(" LIMIT %1").arg(limit));
        }
        if (!insertQuery.prepare(insertStatement)) {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare temporary contact ids: %1\n%2")
                    .arg(insertQuery.lastError().text())
                    .arg(insertStatement));
            return false;
        }
        bindValues(insertQuery, boundValues);
        if (!ContactsDatabase::execute(insertQuery)) {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to insert temporary contact ids: %1\n%2")
                    .arg(insertQuery.lastError().text())
                    .arg(insertStatement));
            return false;
        } else {
            debugFilterExpansion("Contacts selection:", insertStatement, boundValues);
        }
        insertQuery.finish();
    } else {
        // specified by id list
        // NOTE: we must preserve the order of the bound ids being
        // inserted (to match the order of the input list), so that
        // the result of queryContacts() is ordered according to the
        // order of input ids.
        if (!boundIds.isEmpty()) {
            QVariantList::const_iterator it = boundIds.constBegin(), end = boundIds.constEnd();
            if ((limit > 0) && (limit < boundIds.count())) {
                end = it + limit;
            }
            while (it != end) {
                // SQLite allows up to 500 rows per insert
                quint32 remainder = (end - it);
                QVariantList::const_iterator batchEnd = it + std::min<quint32>(remainder, 500);

                QString insertStatement = QString::fromLatin1("INSERT INTO temp.%1 (contactId) VALUES ").arg(table);
                while (true) {
                    const QVariant &v(*it);
                    const quint32 dbId(v.value<quint32>());
                    insertStatement.append(QString::fromLatin1("(%1)").arg(dbId));
                    if (++it == batchEnd) {
                        break;
                    } else {
                        insertStatement.append(QString::fromLatin1(","));
                    }
                }

                if (!insertQuery.prepare(insertStatement)) {
                    QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare temporary contact ids: %1\n%2")
                            .arg(insertQuery.lastError().text())
                            .arg(insertStatement));
                    return false;
                }
                if (!ContactsDatabase::execute(insertQuery)) {
                    QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to insert temporary contact ids: %1\n%2")
                            .arg(insertQuery.lastError().text())
                            .arg(insertStatement));
                    return false;
                }
                insertQuery.finish();
            }
        }
    }

    return true;
}

void dropOrDeleteTable(QSqlDatabase &db, const QString &table)
{
    QSqlQuery dropTableQuery(db);
    const QString dropTableStatement = QString::fromLatin1("DROP TABLE IF EXISTS temp.%1").arg(table);
    if (!dropTableQuery.prepare(dropTableStatement) || !ContactsDatabase::execute(dropTableQuery)) {
        // couldn't drop the table, just delete all entries instead.
        QSqlQuery deleteRecordsQuery(db);
        const QString deleteRecordsStatement = QString::fromLatin1("DELETE FROM temp.%1").arg(table);
        if (!deleteRecordsQuery.prepare(deleteRecordsStatement)) {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare delete records query - the next query may return spurious results: %1\n%2")
                    .arg(deleteRecordsQuery.lastError().text())
                    .arg(deleteRecordsStatement));
        }
        if (!ContactsDatabase::execute(deleteRecordsQuery)) {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to delete temporary records - the next query may return spurious results: %1\n%2")
                    .arg(deleteRecordsQuery.lastError().text())
                    .arg(deleteRecordsStatement));
        }
    }
}

void clearTemporaryContactIdsTable(QSqlDatabase &db, const QString &table)
{
    // Drop any transient tables associated with this table
    dropTransientTables(db, table);

    dropOrDeleteTable(db, table);
}

bool createTemporaryContactTimestampTable(QSqlDatabase &db, const QString &table, const QList<QPair<quint32, QString> > &values)
{
    static const QString createStatement(QString::fromLatin1("CREATE TABLE IF NOT EXISTS temp.%1 ("
                                                                 "contactId INTEGER PRIMARY KEY ASC,"
                                                                 "modified DATETIME"
                                                             ")"));

    // Create the temporary table (if we haven't already).
    QSqlQuery tableQuery(db);
    if (!tableQuery.prepare(createStatement.arg(table))) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare temporary timestamp table query: %1\n%2")
                .arg(tableQuery.lastError().text())
                .arg(createStatement.arg(table)));
        return false;
    }
    if (!ContactsDatabase::execute(tableQuery)) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to create temporary timestamp table: %1\n%2")
                .arg(tableQuery.lastError().text())
                .arg(createStatement.arg(table)));
        return false;
    }
    tableQuery.finish();

    // insert into the temporary table, all of the values
    if (!values.isEmpty()) {
        QSqlQuery insertQuery(db);
        QList<QPair<quint32, QString> >::const_iterator it = values.constBegin(), end = values.constEnd();
        while (it != end) {
            // SQLite/QtSql limits the amount of data we can insert per individual query
            quint32 first = (it - values.constBegin());
            quint32 remainder = (end - it);
            quint32 count = std::min<quint32>(remainder, 250);
            QList<QPair<quint32, QString> >::const_iterator batchEnd = it + count;

            QString insertStatement = QString::fromLatin1("INSERT INTO temp.%1 (contactId, modified) VALUES ").arg(table);
            while (true) {
                insertStatement.append(QString::fromLatin1("(?,?)"));
                if (++it == batchEnd) {
                    break;
                } else {
                    insertStatement.append(QString::fromLatin1(","));
                }
            }

            if (!insertQuery.prepare(insertStatement)) {
                QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare temporary timestamp values: %1\n%2")
                        .arg(insertQuery.lastError().text())
                        .arg(insertStatement));
                return false;
            }

            QList<QPair<quint32, QString> >::const_iterator vit = values.constBegin() + first, vend = vit + count;
            while (vit != vend) {
                const QPair<quint32, QString> &pair(*vit);
                ++vit;

                insertQuery.addBindValue(QVariant(pair.first));
                insertQuery.addBindValue(QVariant(pair.second));
            }

            if (!ContactsDatabase::execute(insertQuery)) {
                QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to insert temporary timestamp values: %1\n%2")
                        .arg(insertQuery.lastError().text())
                        .arg(insertStatement));
                return false;
            }
            insertQuery.finish();
        }
    }

    return true;
}

void clearTemporaryContactTimestampTable(QSqlDatabase &db, const QString &table)
{
    dropOrDeleteTable(db, table);
}

bool createTemporaryContactPresenceTable(QSqlDatabase &db, const QString &table, const QList<QPair<quint32, qint64> > &values)
{
    static const QString createStatement(QString::fromLatin1("CREATE TABLE IF NOT EXISTS temp.%1 ("
                                                                 "contactId INTEGER PRIMARY KEY ASC,"
                                                                 "presenceState INTEGER,"
                                                                 "isOnline BOOL"
                                                             ")"));

    // Create the temporary table (if we haven't already).
    QSqlQuery tableQuery(db);
    if (!tableQuery.prepare(createStatement.arg(table))) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare temporary presence table query: %1\n%2")
                .arg(tableQuery.lastError().text())
                .arg(createStatement.arg(table)));
        return false;
    }
    if (!ContactsDatabase::execute(tableQuery)) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to create temporary presence table: %1\n%2")
                .arg(tableQuery.lastError().text())
                .arg(createStatement.arg(table)));
        return false;
    }
    tableQuery.finish();

    // insert into the temporary table, all of the values
    if (!values.isEmpty()) {
        QSqlQuery insertQuery(db);
        QList<QPair<quint32, qint64> >::const_iterator it = values.constBegin(), end = values.constEnd();
        while (it != end) {
            // SQLite/QtSql limits the amount of data we can insert per individual query
            quint32 first = (it - values.constBegin());
            quint32 remainder = (end - it);
            quint32 count = std::min<quint32>(remainder, 167);
            QList<QPair<quint32, qint64> >::const_iterator batchEnd = it + count;

            QString insertStatement = QString::fromLatin1("INSERT INTO temp.%1 (contactId, presenceState, isOnline) VALUES ").arg(table);
            while (true) {
                insertStatement.append(QString::fromLatin1("(?,?,?)"));
                if (++it == batchEnd) {
                    break;
                } else {
                    insertStatement.append(QString::fromLatin1(","));
                }
            }

            if (!insertQuery.prepare(insertStatement)) {
                QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare temporary presence values: %1\n%2")
                        .arg(insertQuery.lastError().text())
                        .arg(insertStatement));
                return false;
            }

            QList<QPair<quint32, qint64> >::const_iterator vit = values.constBegin() + first, vend = vit + count;
            while (vit != vend) {
                const QPair<quint32, qint64> &pair(*vit);
                ++vit;

                insertQuery.addBindValue(QVariant(pair.first));

                const int state(pair.second);
                insertQuery.addBindValue(QVariant(state));
                insertQuery.addBindValue(QVariant(state >= QContactPresence::PresenceAvailable && state <= QContactPresence::PresenceExtendedAway));
            }

            if (!ContactsDatabase::execute(insertQuery)) {
                QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to insert temporary presence values: %1\n%2")
                        .arg(insertQuery.lastError().text())
                        .arg(insertStatement));
                return false;
            }
            insertQuery.finish();
        }
    }

    return true;
}

void clearTemporaryContactPresenceTable(QSqlDatabase &db, const QString &table)
{
    dropOrDeleteTable(db, table);
}

bool createTemporaryValuesTable(QSqlDatabase &db, const QString &table, const QVariantList &values)
{
    static const QString createStatement(QString::fromLatin1("CREATE TABLE IF NOT EXISTS temp.%1 (value BLOB)"));

    // Create the temporary table (if we haven't already).
    QSqlQuery tableQuery(db);
    if (!tableQuery.prepare(createStatement.arg(table))) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare temporary table query: %1\n%2")
                .arg(tableQuery.lastError().text())
                .arg(createStatement));
        return false;
    }
    if (!ContactsDatabase::execute(tableQuery)) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to create temporary table: %1\n%2")
                .arg(tableQuery.lastError().text())
                .arg(createStatement));
        return false;
    }
    tableQuery.finish();

    // insert into the temporary table, all of the values
    if (!values.isEmpty()) {
        QSqlQuery insertQuery(db);
        QVariantList::const_iterator it = values.constBegin(), end = values.constEnd();
        while (it != end) {
            // SQLite/QtSql limits the amount of data we can insert per individual query
            quint32 first = (it - values.constBegin());
            quint32 remainder = (end - it);
            quint32 count = std::min<quint32>(remainder, 500);
            QVariantList::const_iterator batchEnd = it + count;

            QString insertStatement = QString::fromLatin1("INSERT INTO temp.%1 (value) VALUES ").arg(table);
            while (true) {
                insertStatement.append(QString::fromLatin1("(?)"));
                if (++it == batchEnd) {
                    break;
                } else {
                    insertStatement.append(QString::fromLatin1(","));
                }
            }

            if (!insertQuery.prepare(insertStatement)) {
                QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare temporary values: %1\n%2")
                        .arg(insertQuery.lastError().text())
                        .arg(insertStatement));
                return false;
            }

            foreach (const QVariant &v, values.mid(first, count)) {
                insertQuery.addBindValue(v);
            }

            if (!ContactsDatabase::execute(insertQuery)) {
                QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to insert temporary values: %1\n%2")
                        .arg(insertQuery.lastError().text())
                        .arg(insertStatement));
                return false;
            }
            insertQuery.finish();
        }
    }

    return true;
}

void clearTemporaryValuesTable(QSqlDatabase &db, const QString &table)
{
    dropOrDeleteTable(db, table);
}

static bool createTransientContactIdsTable(QSqlDatabase &db, const QString &table, const QVariantList &ids, QString *transientTableName)
{
    static const QString createTableStatement(QString::fromLatin1("CREATE TABLE %1 (contactId INTEGER)"));
    static const QString insertIdsStatement(QString::fromLatin1("INSERT INTO %1 (contactId) VALUES(:contactId)"));

    int existingTables = 0;
    if (!countTransientTables(db, table, &existingTables))
        return false;

    QString tableName(QString::fromLatin1("temp.%1_transient%2").arg(table).arg(existingTables));

    QSqlQuery tableQuery(db);
    const QString createStatement(createTableStatement.arg(tableName));
    if (!tableQuery.prepare(createStatement)) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare transient table query: %1\n%2")
                .arg(tableQuery.lastError().text())
                .arg(createStatement));
        return false;
    }
    if (!ContactsDatabase::execute(tableQuery)) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to create transient table: %1\n%2")
                .arg(tableQuery.lastError().text())
                .arg(createStatement));
        return false;
    }
    tableQuery.finish();

    QSqlQuery insertQuery(db);

    QVariantList::const_iterator it = ids.constBegin(), end = ids.constEnd();
    while (it != end) {
        // SQLite allows up to 500 rows per insert
        quint32 remainder = (end - it);
        QVariantList::const_iterator batchEnd = it + std::min<quint32>(remainder, 500);

        QString insertStatement = QString::fromLatin1("INSERT INTO %1 (contactId) VALUES ").arg(tableName);
        while (true) {
            const QVariant &v(*it);
            const quint32 dbId(v.value<quint32>());
            insertStatement.append(QString::fromLatin1("(%1)").arg(dbId));
            if (++it == batchEnd) {
                break;
            } else {
                insertStatement.append(QString::fromLatin1(","));
            }
        }

        if (!insertQuery.prepare(insertStatement)) {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare transient contact ids: %1\n%2")
                    .arg(insertQuery.lastError().text())
                    .arg(insertStatement));
            return false;
        }
        if (!ContactsDatabase::execute(insertQuery)) {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to insert transient contact ids: %1\n%2")
                    .arg(insertQuery.lastError().text())
                    .arg(insertStatement));
            return false;
        }
        insertQuery.finish();
    }

    *transientTableName = tableName;
    return true;
}

static const int initialSemaphoreValues[] = { 1, 0, 1 };

static size_t databaseOwnershipIndex = 0;
static size_t databaseConnectionsIndex = 1;
static size_t writeAccessIndex = 2;

// Adapted from the inter-process mutex in QMF
// The first user creates the semaphore that all subsequent instances
// attach to.  We rely on undo semantics to release locked semaphores
// on process failure.
ContactsDatabase::ProcessMutex::ProcessMutex(const QString &path)
    : m_semaphore(path.toLatin1(), 3, initialSemaphoreValues)
    , m_initialProcess(false)
{
    if (!m_semaphore.isValid()) {
        QTCONTACTS_SQLITE_WARNING(QStringLiteral("Unable to create semaphore array!"));
    } else {
        if (!m_semaphore.decrement(databaseOwnershipIndex)) {
            QTCONTACTS_SQLITE_WARNING(QStringLiteral("Unable to determine database ownership!"));
        } else {
            // Only the first process to connect to the semaphore is the owner
            m_initialProcess = (m_semaphore.value(databaseConnectionsIndex) == 0);
            if (!m_semaphore.increment(databaseConnectionsIndex)) {
                QTCONTACTS_SQLITE_WARNING(QStringLiteral("Unable to increment database connections!"));
            }

            m_semaphore.increment(databaseOwnershipIndex);
        }
    }
}

bool ContactsDatabase::ProcessMutex::lock()
{
    return m_semaphore.decrement(writeAccessIndex);
}

bool ContactsDatabase::ProcessMutex::unlock()
{
    return m_semaphore.increment(writeAccessIndex);
}

bool ContactsDatabase::ProcessMutex::isLocked() const
{
    return (m_semaphore.value(writeAccessIndex) == 0);
}

bool ContactsDatabase::ProcessMutex::isInitialProcess() const
{
    return m_initialProcess;
}

ContactsDatabase::Query::Query(const QSqlQuery &query)
    : m_query(query)
{
}

void ContactsDatabase::Query::reportError(const QString &text) const
{
    QString output(text + QString::fromLatin1("\n%1").arg(m_query.lastError().text()));
    QTCONTACTS_SQLITE_WARNING(output);
}

void ContactsDatabase::Query::reportError(const char *text) const
{
    reportError(QString::fromLatin1(text));
}

ContactsDatabase::ContactsDatabase()
    : m_mutex(QMutex::Recursive)
    , m_nonprivileged(false)
    , m_localeName(QLocale().name())
{
}

ContactsDatabase::~ContactsDatabase()
{
    m_database.close();
}

QMutex *ContactsDatabase::accessMutex() const
{
    return const_cast<QMutex *>(&m_mutex);
}

ContactsDatabase::ProcessMutex *ContactsDatabase::processMutex() const
{
    if (!m_processMutex) {
        Q_ASSERT(m_database.isOpen());
        m_processMutex.reset(new ProcessMutex(m_database.databaseName()));
    }
    return m_processMutex.data();
}

// QDir::isReadable() doesn't support group permissions, only user permissions.
bool directoryIsRW(const QString &dirPath)
{
  QFileInfo databaseDirInfo(dirPath);
  return (databaseDirInfo.permission(QFile::ReadGroup | QFile::WriteGroup)
       || databaseDirInfo.permission(QFile::ReadUser  | QFile::WriteUser));
}

bool ContactsDatabase::open(const QString &connectionName, bool nonprivileged, bool secondaryConnection)
{
    QMutexLocker locker(accessMutex());

    if (m_database.isOpen()) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Unable to open database when already open: %1").arg(connectionName));
        return false;
    }

    // horrible hack: Qt4 didn't have GenericDataLocation so we hardcode DATA_DIR location.
    QString privilegedDataDir(QString("%1/%2/")
            .arg(QString::fromLatin1(QTCONTACTS_SQLITE_CENTRAL_DATA_DIR))
            .arg(QString::fromLatin1(QTCONTACTS_SQLITE_PRIVILEGED_DIR)));
    QString unprivilegedDataDir(QString::fromLatin1(QTCONTACTS_SQLITE_CENTRAL_DATA_DIR));

    QDir databaseDir;
    if (!nonprivileged && databaseDir.mkpath(privilegedDataDir + QString::fromLatin1(QTCONTACTS_SQLITE_DATABASE_DIR))) {
        // privileged.
        databaseDir = privilegedDataDir + QString::fromLatin1(QTCONTACTS_SQLITE_DATABASE_DIR);
    } else {
        // not privileged.
        if (!databaseDir.mkpath(unprivilegedDataDir + QString::fromLatin1(QTCONTACTS_SQLITE_DATABASE_DIR))) {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Unable to create contacts database directory: %1").arg(unprivilegedDataDir + QString::fromLatin1(QTCONTACTS_SQLITE_DATABASE_DIR)));
            return false;
        }
        databaseDir = unprivilegedDataDir + QString::fromLatin1(QTCONTACTS_SQLITE_DATABASE_DIR);
        if (!nonprivileged) {
            QTCONTACTS_SQLITE_DEBUG(QString::fromLatin1("Could not access privileged data directory; using nonprivileged"));
        }
        m_nonprivileged = true;
    }

    const QString databaseFile = databaseDir.absoluteFilePath(QString::fromLatin1(QTCONTACTS_SQLITE_DATABASE_NAME));
    const bool databasePreexisting = QFile::exists(databaseFile);
    if (!databasePreexisting && secondaryConnection) {
        // The database must already be created/checked/opened by a primary connection
        return false;
    }

    m_database = QSqlDatabase::addDatabase(QString::fromLatin1("QSQLITE"), connectionName);
    m_database.setDatabaseName(databaseFile);

    if (!m_database.open()) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to open contacts database: %1")
                .arg(m_database.lastError().text()));
        return false;
    }

    if (!databasePreexisting && !prepareDatabase(m_database, aggregating(), m_localeName)) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare contacts database - removing: %1")
                .arg(m_database.lastError().text()));

        m_database.close();
        QFile::remove(databaseFile);
        return false;
    } else if (databasePreexisting && !configureDatabase(m_database, m_localeName)) {
        m_database.close();
        return false;
    }

    // Get the process mutex for this database
    ProcessMutex *mutex(processMutex());

    // Only the first connection in the first process to concurrently open the DB is the owner
    const bool databaseOwner(!secondaryConnection && mutex->isInitialProcess());

    if (databasePreexisting && databaseOwner) {
        // Try to upgrade, if necessary
        if (mutex->lock()) {
            // Perform an integrity check
            if (!checkDatabase(m_database)) {
                QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to check integrity of contacts database: %1")
                        .arg(m_database.lastError().text()));
                m_database.close();
                mutex->unlock();
                return false;
            }

            if (!upgradeDatabase(m_database)) {
                QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to upgrade contacts database: %1")
                        .arg(m_database.lastError().text()));
                m_database.close();
                mutex->unlock();
                return false;
            }

            mutex->unlock();
        } else {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to lock mutex for contacts database: %1")
                    .arg(databaseFile));
            m_database.close();
            return false;
        }
    }

    // Attach to the transient store - any process can create it, but only the primary connection of each
    if (!m_transientStore.open(nonprivileged, !secondaryConnection, !databasePreexisting)) {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to open contacts transient store"));
        m_database.close();
        return false;
    }

    QTCONTACTS_SQLITE_DEBUG(QString::fromLatin1("Opened contacts database: %1 Locale: %2").arg(databaseFile).arg(m_localeName));
    return true;
}

ContactsDatabase::operator QSqlDatabase &()
{
    return m_database;
}

ContactsDatabase::operator QSqlDatabase const &() const
{
    return m_database;
}

QSqlError ContactsDatabase::lastError() const
{
    return m_database.lastError();
}

bool ContactsDatabase::nonprivileged() const
{
    return m_nonprivileged;
}

bool ContactsDatabase::localized() const
{
    return (m_localeName != QLatin1String("C"));
}

bool ContactsDatabase::aggregating() const
{
    // Currently true only in the privileged database
    return !m_nonprivileged;
}

bool ContactsDatabase::beginTransaction()
{
    ProcessMutex *mutex(processMutex());

    // We use a cross-process mutex to ensure only one process can write
    // to the DB at once.  Without external locking, SQLite will back off
    // on write contention, and the backed-off process may never get access
    // if other processes are performing regular writes.
    if (mutex->lock()) {
        if (::beginTransaction(m_database))
            return true;

        mutex->unlock();
    }

    return false;
}

bool ContactsDatabase::commitTransaction()
{
    ProcessMutex *mutex(processMutex());

    if (::commitTransaction(m_database)) {
        if (mutex->isLocked()) {
            mutex->unlock();
        } else {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Lock error: no lock held on commit"));
        }
        return true;
    }

    return false;
}

bool ContactsDatabase::rollbackTransaction()
{
    ProcessMutex *mutex(processMutex());

    const bool rv = ::rollbackTransaction(m_database);

    if (mutex->isLocked()) {
        mutex->unlock();
    } else {
        QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Lock error: no lock held on rollback"));
    }

    return rv;
}

ContactsDatabase::Query ContactsDatabase::prepare(const char *statement)
{
    return prepare(QString::fromLatin1(statement));
}

ContactsDatabase::Query ContactsDatabase::prepare(const QString &statement)
{
    QMutexLocker locker(accessMutex());

    QHash<QString, QSqlQuery>::const_iterator it = m_preparedQueries.constFind(statement);
    if (it == m_preparedQueries.constEnd()) {
        QSqlQuery query(m_database);
        query.setForwardOnly(true);
        if (!query.prepare(statement)) {
            QTCONTACTS_SQLITE_WARNING(QString::fromLatin1("Failed to prepare query: %1\n%2")
                    .arg(query.lastError().text())
                    .arg(statement));
            return Query(QSqlQuery());
        }
        it = m_preparedQueries.insert(statement, query);
    }

    return Query(*it);
}

bool ContactsDatabase::hasTransientDetails(quint32 contactId)
{
    return m_transientStore.contains(contactId);
}

QPair<QDateTime, QList<QContactDetail> > ContactsDatabase::transientDetails(quint32 contactId) const
{
    return m_transientStore.contactDetails(contactId);
}

bool ContactsDatabase::setTransientDetails(quint32 contactId, const QDateTime &timestamp, const QList<QContactDetail> &details)
{
    return m_transientStore.setContactDetails(contactId, timestamp, details);
}

bool ContactsDatabase::removeTransientDetails(quint32 contactId)
{
    return m_transientStore.remove(contactId);
}

bool ContactsDatabase::removeTransientDetails(const QList<quint32> &contactIds)
{
    return m_transientStore.remove(contactIds);
}

bool ContactsDatabase::execute(QSqlQuery &query)
{
    static const bool debugSql = !qgetenv("QTCONTACTS_SQLITE_DEBUG_SQL").isEmpty();

    QElapsedTimer t;
    t.start();

    bool rv = query.exec();
    if (debugSql && rv) {
        const int n = query.isSelect() ? query.size() : query.numRowsAffected();
        const QString s(expandQuery(query));
        qDebug().nospace() << "Query in " << t.elapsed() << "ms " << n << ": " << qPrintable(s);
    }

    return rv;
}

bool ContactsDatabase::executeBatch(QSqlQuery &query, QSqlQuery::BatchExecutionMode mode)
{
    static const bool debugSql = !qgetenv("QTCONTACTS_SQLITE_DEBUG_SQL").isEmpty();

    QElapsedTimer t;
    t.start();

    bool rv = query.execBatch(mode);
    if (debugSql && rv) {
        const int n = query.isSelect() ? query.size() : query.numRowsAffected();
        const QString s(expandQuery(query));
        qDebug().nospace() << "Batch query in " << t.elapsed() << "ms " << n << ": " << qPrintable(s);
    }

    return rv;
}

QString ContactsDatabase::expandQuery(const QString &queryString, const QVariantList &bindings)
{
    QString query(queryString);

    int index = 0;
    for (int i = 0; i < bindings.count(); ++i) {
        static const QChar marker = QChar::fromLatin1('?');

        QString value = bindings.at(i).toString();
        index = query.indexOf(marker, index);
        if (index == -1)
            break;

        query.replace(index, 1, value);
        index += value.length();
    }

    return query;
}

QString ContactsDatabase::expandQuery(const QString &queryString, const QMap<QString, QVariant> &bindings)
{
    QString query(queryString);

    int index = 0;

    while (true) {
        static const QChar marker = QChar::fromLatin1(':');

        index = query.indexOf(marker, index);
        if (index == -1)
            break;

        int remaining = query.length() - index;
        int len = 1;
        for ( ; (len < remaining) && query.at(index + len).isLetter(); ) {
            ++len;
        }

        const QString key(query.mid(index, len));
        QVariant value = bindings.value(key);

        QString valueText;
        if (value.type() == QVariant::String) {
            valueText = QString::fromLatin1("'%1'").arg(value.toString());
        } else {
            valueText = value.toString();
        }

        query.replace(index, len, valueText);
        index += valueText.length();
    }

    return query;
}

QString ContactsDatabase::expandQuery(const QSqlQuery &query)
{
    return expandQuery(query.lastQuery(), query.boundValues());
}

bool ContactsDatabase::createTemporaryContactIdsTable(const QString &table, const QVariantList &boundIds, int limit)
{
    QMutexLocker locker(accessMutex());
    return ::createTemporaryContactIdsTable(m_database, table, false, boundIds, QString(), QString(), QString(), QVariantList(), limit);
}

bool ContactsDatabase::createTemporaryContactIdsTable(const QString &table, const QString &join, const QString &where, const QString &orderBy, const QVariantList &boundValues, int limit)
{
    QMutexLocker locker(accessMutex());
    return ::createTemporaryContactIdsTable(m_database, table, true, QVariantList(), join, where, orderBy, boundValues, limit);
}

bool ContactsDatabase::createTemporaryContactIdsTable(const QString &table, const QString &join, const QString &where, const QString &orderBy, const QMap<QString, QVariant> &boundValues, int limit)
{
    QMutexLocker locker(accessMutex());
    return ::createTemporaryContactIdsTable(m_database, table, true, QVariantList(), join, where, orderBy, boundValues, limit);
}

void ContactsDatabase::clearTemporaryContactIdsTable(const QString &table)
{
    QMutexLocker locker(accessMutex());
    ::clearTemporaryContactIdsTable(m_database, table);
}

bool ContactsDatabase::createTemporaryValuesTable(const QString &table, const QVariantList &values)
{
    QMutexLocker locker(accessMutex());
    return ::createTemporaryValuesTable(m_database, table, values);
}

void ContactsDatabase::clearTemporaryValuesTable(const QString &table)
{
    QMutexLocker locker(accessMutex());
    ::clearTemporaryValuesTable(m_database, table);
}

bool ContactsDatabase::createTransientContactIdsTable(const QString &table, const QVariantList &ids, QString *transientTableName)
{
    QMutexLocker locker(accessMutex());
    return ::createTransientContactIdsTable(m_database, table, ids, transientTableName);
}

void ContactsDatabase::clearTransientContactIdsTable(const QString &table)
{
    QMutexLocker locker(accessMutex());
    ::dropTransientTables(m_database, table);
}

bool ContactsDatabase::populateTemporaryTransientState(bool timestamps, bool globalPresence)
{
    const QString timestampTable(QStringLiteral("Timestamps"));
    const QString presenceTable(QStringLiteral("GlobalPresenceStates"));

    QMutexLocker locker(accessMutex());

    if (timestamps) {
        ::clearTemporaryContactTimestampTable(m_database, timestampTable);
    }
    if (globalPresence) {
        ::clearTemporaryContactPresenceTable(m_database, presenceTable);
    }

    // Find the current temporary states from transient storage
    QList<QPair<quint32, qint64> > presenceValues;
    QList<QPair<quint32, QString> > timestampValues;

    {
        ContactsTransientStore::DataLock lock(m_transientStore.dataLock());
        ContactsTransientStore::const_iterator it = m_transientStore.constBegin(lock), end = m_transientStore.constEnd(lock);
        for ( ; it != end; ++it) {
            QPair<QDateTime, QList<QContactDetail> > details(it.value());
            if (details.first.isNull())
                continue;

            if (timestamps) {
                timestampValues.append(qMakePair<quint32, QString>(it.key(), dateTimeString(details.first)));
            }

            if (globalPresence) {
                foreach (const QContactDetail &detail, details.second) {
                    if (detail.type() == QContactGlobalPresence::Type) {
                        presenceValues.append(qMakePair<quint32, qint64>(it.key(), detail.value<int>(QContactGlobalPresence::FieldPresenceState)));
                        break;
                    }
                }
            }
        }
    }

    bool rv = true;
    if (timestamps && !::createTemporaryContactTimestampTable(m_database, timestampTable, timestampValues)) {
        rv = false;
    } else if (globalPresence && !::createTemporaryContactPresenceTable(m_database, presenceTable, presenceValues)) {
        rv = false;
    }
    return rv;
}

QString ContactsDatabase::dateTimeString(const QDateTime &qdt)
{
    // Input must be UTC
    return qdt.toString(QStringLiteral("yyyy-MM-ddThh:mm:ss.zzz"));
}

QString ContactsDatabase::dateString(const QDateTime &qdt)
{
    // Input must be UTC
    return qdt.toString(QStringLiteral("yyyy-MM-dd"));
}

QDateTime ContactsDatabase::fromDateTimeString(const QString &s)
{
    QDateTime rv(QDateTime::fromString(s, QStringLiteral("yyyy-MM-ddThh:mm:ss.zzz")));
    rv.setTimeSpec(Qt::UTC);
    return rv;
}

#include "../extensions/qcontactdeactivated_impl.h"
#include "../extensions/qcontactincidental_impl.h"
#include "../extensions/qcontactoriginmetadata_impl.h"
#include "../extensions/qcontactstatusflags_impl.h"
