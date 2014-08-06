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

#include <QContactManager>
#include <QContactFetchRequest>
#include <QContactFavorite>
#include <QContactName>
#include <QContactEmailAddress>
#include <QContactPhoneNumber>
#include <QContactDisplayLabel>
#include <QContactHobby>
#include <QContactAvatar>
#include <QContactAddress>
#include <QContactPresence>
#include <QContactNickname>
#include <QContactOnlineAccount>
#include <QContactSyncTarget>
#include <QContactDetailFilter>
#include <QContactFetchHint>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QtDebug>

QTCONTACTS_USE_NAMESPACE

#include <QContactIdFilter>
static QContactId retrievalId(const QContact &contact) { return contact.id(); }

static QStringList generateNonOverlappingFirstNamesList()
{
    QStringList retn;
    retn << "Zach" << "Zane" << "Zinedine" << "Zockey"
         << "Yann" << "Yedrez" << "Yarrow" << "Yelter"
         << "Ximmy" << "Xascha" << "Xanthar" << "Xachy"
         << "William" << "Wally" << "Weston" << "Wulther"
         << "Vernon" << "Veston" << "Victoria" << "Vuitton"
         << "Urqhart" << "Uelela" << "Ulrich" << "Umpty"
         << "Timothy" << "Tigga" << "Tabitha" << "Texter"
         << "Stan" << "Steve" << "Sophie" << "Siphonie"
         << "Richard" << "Rafael" << "Rachael" << "Rascal"
         << "Quirky" << "Quilton" << "Quentin" << "Quarreller";
    return retn;
}

static QStringList generateNonOverlappingLastNamesList()
{
    QStringList retn;
    retn << "Quilter" << "Qualifa" << "Quarrier" << "Quickson"
         << "Rigger" << "Render" << "Ranger" << "Reader"
         << "Sailor" << "Smith" << "Salter" << "Shelfer"
         << "Tailor" << "Tasker" << "Toppler" << "Tipster"
         << "Underhill" << "Umpire" << "Upperhill" << "Uppsland"
         << "Vintner" << "Vester" << "Victor" << "Vacationer"
         << "Wicker" << "Whaler" << "Whistler" << "Wolf"
         << "Xylophone" << "Xabu" << "Xanadu" << "Xatti"
         << "Yeoman" << "Yesman" << "Yelper" << "Yachtsman"
         << "Zimmerman" << "Zomething" << "Zeltic" << "Zephyr";
    return retn;
}

static QStringList generateFirstNamesList()
{
    QStringList retn;
    retn << "Alexandria" << "Andrew" << "Adrien" << "Amos"
         << "Bob" << "Bronte" << "Barry" << "Braxton"
         << "Clarence" << "Chandler" << "Chris" << "Chantelle"
         << "Dominic" << "Diedre" << "David" << "Derrick"
         << "Eric" << "Esther" << "Eddie" << "Eean"
         << "Felicity" << "Fred" << "Fletcher" << "Farraday"
         << "Gary" << "Gertrude" << "Gerry" << "Germaine"
         << "Hillary" << "Henry" << "Hans" << "Haddock"
         << "Jacob" << "Jane" << "Jackson" << "Jennifer"
         << "Larry" << "Lilliane" << "Lambert" << "Lilly"
         << "Mary" << "Mark" << "Mirriam" << "Matthew"
         << "Nathene" << "Nicholas" << "Ned" << "Norris"
         << "Othello" << "Oscar" << "Olaf" << "Odinsdottur"
         << "Penny" << "Peter" << "Patrick" << "Pilborough";
    return retn;
}

static QStringList generateMiddleNamesList()
{
    QStringList retn;
    retn << "Aubrey" << "Cody" << "Taylor" << "Leslie";
    return retn;
}

static QStringList generateLastNamesList()
{
    QStringList retn;
    retn << "Arkady" << "Addleman" << "Axeman" << "Applegrower" << "Anderson"
         << "Baker" << "Bremmer" << "Bedlam" << "Barrymore" << "Battery"
         << "Cutter" << "Cooper" << "Cutler" << "Catcher" << "Capemaker"
         << "Driller" << "Dyer" << "Diver" << "Daytona" << "Duster"
         << "Eeler" << "Eckhart" << "Eggsman" << "Empty" << "Ellersly"
         << "Farmer" << "Farrier" << "Foster" << "Farseer" << "Fairtime"
         << "Grower" << "Gaston" << "Gerriman" << "Gipsland" << "Guilder"
         << "Helper" << "Hogfarmer" << "Harriet" << "Hope" << "Huxley"
         << "Inker" << "Innman" << "Ipland" << "Instiller" << "Innis"
         << "Joker" << "Jackson" << "Jolt" << "Jockey" << "Jerriman";
    return retn;
}

static QStringList generatePhoneNumbersList()
{
    QStringList retn;
    retn << "111222" << "111333" << "111444" << "111555" << "111666"
         << "111777" << "111888" << "111999" << "222333" << "222444"
         << "222555" << "222666" << "222777" << "222888" << "222999"
         << "333444" << "333555" << "333666" << "333777" << "333888"
         << "333999" << "444555" << "444666" << "444777" << "444888"
         << "444999" << "555666" << "555777" << "555888" << "555999"
         << "666111" << "666222" << "666333" << "666444" << "666555"
         << "777111" << "777222" << "777333" << "777444" << "777555"
         << "777666" << "888111" << "888222" << "888333" << "888444"
         << "888555" << "888666" << "888777" << "999111" << "999222"
         << "999333" << "999444" << "999555" << "999666" << "999777"
         << "999888" << "999999";
    return retn;
}

static QStringList generateEmailProvidersList()
{
    QStringList retn;
    retn << "@test.com" << "@testing.com" << "@testers.com"
         << "@test.org" << "@testing.org" << "@testers.org"
         << "@test.net" << "@testing.net" << "@testers.net"
         << "@test.fi" << "@testing.fi" << "@testers.fi"
         << "@test.com.au" << "@testing.com.au" << "@testers.com.au"
         << "@test.co.uk" << "@testing.co.uk" << "@testers.co.uk"
         << "@test.co.jp" << "@test.co.jp" << "@testers.co.jp";
    return retn;
}

static QStringList generateAvatarsList()
{
    QStringList retn;
    retn << "-smiling.jpg" << "-laughing.jpg" << "-surprised.jpg"
         << "-smiling.png" << "-laughing.png" << "-surprised.png"
         << "-curious.jpg" << "-joking.jpg" << "-grinning.jpg"
         << "-curious.png" << "-joking.png" << "-grinning.png";
    return retn;
}

static QStringList generateHobbiesList()
{
    QStringList retn;
    retn << "tennis" << "soccer" << "squash" << "volleyball"
         << "chess" << "photography" << "painting" << "sketching";
    return retn;
}

QContact generateContact(const QString &syncTarget = QString(QLatin1String("local")), bool possiblyAggregate = false)
{
    static const QStringList firstNames(generateFirstNamesList());
    static const QStringList middleNames(generateMiddleNamesList());
    static const QStringList lastNames(generateLastNamesList());
    static const QStringList nonOverlappingFirstNames(generateNonOverlappingFirstNamesList());
    static const QStringList nonOverlappingLastNames(generateNonOverlappingLastNamesList());
    static const QStringList phoneNumbers(generatePhoneNumbersList());
    static const QStringList emailProviders(generateEmailProvidersList());
    static const QStringList avatars(generateAvatarsList());
    static const QStringList hobbies(generateHobbiesList());

    // we randomly determine whether to generate various details
    // to ensure that we have heterogeneous contacts in the db.
    QContact retn;
    int random = qrand();
    bool preventAggregate = (syncTarget != QLatin1String("local") && !possiblyAggregate);

    // We always have a sync target.
    QContactSyncTarget synctarget;
    synctarget.setSyncTarget(syncTarget);
    retn.saveDetail(&synctarget);

    // We always have a name.  Select an overlapping name if the sync target
    // is something other than "local" and possiblyAggregate is true.
    QContactName name;
    name.setFirstName(preventAggregate ?
            nonOverlappingFirstNames.at(random % nonOverlappingFirstNames.size()) :
            firstNames.at(random % firstNames.size()));
    name.setLastName(preventAggregate ?
            nonOverlappingLastNames.at(random % nonOverlappingLastNames.size()) :
            lastNames.at(random % lastNames.size()));
    if ((random % 6) == 0) name.setMiddleName(middleNames.at(random % middleNames.size()));
    if ((random % 17) == 0) name.setPrefix(QLatin1String("Dr."));
    retn.saveDetail(&name);

    // Favorite
    if ((random % 31) == 0) {
        QContactFavorite fav;
        fav.setFavorite(true);
        retn.saveDetail(&fav);
    }

    // Phone number
    if ((random % 3) == 0) {
        QContactPhoneNumber phn;
        QString randomPhn = phoneNumbers.at(random % phoneNumbers.size());
        phn.setNumber(preventAggregate ? QString(QString::number(random % 500000) + randomPhn) : randomPhn);
        if ((random % 9) == 0) phn.setContexts(QContactDetail::ContextWork);
        retn.saveDetail(&phn);
    }

    // Email
    if ((random % 2) == 0) {
        QContactEmailAddress em;
        em.setEmailAddress(QString(QLatin1String("%1%2%3%4"))
                .arg(preventAggregate ? QString(QString::number(random % 500000) + syncTarget) : QString())
                .arg(name.firstName()).arg(name.lastName())
                .arg(emailProviders.at(random % emailProviders.size())));
        if (random % 9) em.setContexts(QContactDetail::ContextWork);
        retn.saveDetail(&em);
    }

    // Avatar
    if ((random % 5) == 0) {
        QContactAvatar av;
        av.setImageUrl(name.firstName() + avatars.at(random % avatars.size()));
        retn.saveDetail(&av);
    }

    // Hobby
    if ((random % 21) == 0) {
        QContactHobby h1;
        h1.setHobby(hobbies.at(random % hobbies.size()));
        retn.saveDetail(&h1);

        int newRandom = qrand();
        if ((newRandom % 2) == 0) {
            QContactHobby h2;
            h2.setHobby(hobbies.at(newRandom % hobbies.size()));
            retn.saveDetail(&h2);
        }
    }

    return retn;
}

int main(int argc, char  *argv[])
{
    QCoreApplication application(argc, argv);

    const QStringList &args(application.arguments());

    const bool quickMode(args.contains(QStringLiteral("-q")) || args.contains(QStringLiteral("--quick")));

    QMap<QString, QString> parameters;
    parameters.insert(QString::fromLatin1("mergePresenceChanges"), QString::fromLatin1("false"));

    QContactManager manager(QString::fromLatin1("org.nemomobile.contacts.sqlite"), parameters);

    QContactFetchRequest request;
    request.setManager(&manager);

    // Perform an initial request to ensure the database has been created before we start timing
    request.start();
    request.waitForFinished();

    qint64 elapsedTimeTotal = 0;

    QElapsedTimer asyncTotalTimer;
    asyncTotalTimer.start();

    // Fetch all, no optimization hints
    for (int i = 0; i < 3; ++i) {
        QElapsedTimer timer;
        timer.start();
        request.start();
        request.waitForFinished();

        qint64 elapsed = timer.elapsed();
        qDebug() << i << ": Fetch completed in" << elapsed << "ms";
        elapsedTimeTotal += elapsed;
    }

    // Skip relationships
    QContactFetchHint hint;
    hint.setOptimizationHints(QContactFetchHint::NoRelationships);
    request.setFetchHint(hint);

    for (int i = 0; i < 3; ++i) {
        QElapsedTimer timer;
        timer.start();
        request.start();
        request.waitForFinished();

        qint64 elapsed = timer.elapsed();
        qDebug() << i << ": No-relationships fetch completed in" << elapsed << "ms";
        elapsedTimeTotal += elapsed;
    }

    // Reduce data access
    hint.setDetailTypesHint(QList<QContactDetail::DetailType>() << QContactName::Type << QContactAddress::Type);
    request.setFetchHint(hint);

    for (int i = 0; i < 3; ++i) {
        QElapsedTimer timer;
        timer.start();
        request.start();
        request.waitForFinished();

        qint64 elapsed = timer.elapsed();
        qDebug() << i << ": Reduced data fetch completed in" << elapsed << "ms";
        elapsedTimeTotal += elapsed;
    }

    // Reduce number of results
    hint.setMaxCountHint(request.contacts().count() / 8);
    request.setFetchHint(hint);

    for (int i = 0; i < 3; ++i) {
        QElapsedTimer timer;
        timer.start();
        request.start();
        request.waitForFinished();

        qint64 elapsed = timer.elapsed();
        qDebug() << i << ": Max count fetch completed in" << elapsed << "ms";
        elapsedTimeTotal += elapsed;
    }
    qint64 asyncTotalElapsed = asyncTotalTimer.elapsed();



    // Time some synchronous operations.  First, generate the test data.
    qsrand((int)asyncTotalElapsed);
    QList<int> nbrContacts;
    if (quickMode) {
        nbrContacts << 200;
    } else {
        nbrContacts << 10 << 100 << 500 << 1000 << 2000;
    }
    QList<QList<QContact> > testData;
    qDebug() << "\n\nGenerating test data for timings...";
    for (int i = 0; i < nbrContacts.size(); ++i) {
        int howMany = nbrContacts.at(i);
        QList<QContact> newTestData;
        newTestData.reserve(howMany);

        for (int j = 0; j < howMany; ++j) {
            // Use testing sync target, so 'local' won't be modified into 'was_local' via aggregation
            newTestData.append(generateContact(QString::fromLatin1("testing")));
        }

        testData.append(newTestData);
    }


    // Perform the timings - these all create new contacts and assume an "empty" initial database
    QElapsedTimer syncTimer;
    for (int i = 0; i < testData.size(); ++i) {
        QList<QContact> td = testData.at(i);
        qint64 ste = 0;
        qDebug() << "Performing tests for" << td.size() << "contacts:";

        syncTimer.start();
        manager.saveContacts(&td);
        ste = syncTimer.elapsed();
        qDebug() << "    saving took" << ste << "milliseconds (" << ((1.0 * ste) / (1.0 * td.size())) << "msec per contact )";
        elapsedTimeTotal += ste;

        QContactDetailFilter testingFilter;
        testingFilter.setDetailType(QContactSyncTarget::Type, QContactSyncTarget::FieldSyncTarget);
        testingFilter.setValue(QString::fromLatin1("testing"));

        QContactFetchHint fh;
        syncTimer.start();
        QList<QContact> readContacts = manager.contacts(testingFilter, QList<QContactSortOrder>(), fh);
        ste = syncTimer.elapsed();
        if (readContacts.size() != td.size()) {
            qWarning() << "Invalid retrieval count:" << readContacts.size() << "expecting:" << td.size();
        }
        qDebug() << "    reading all (" << readContacts.size() << "), all details, took" << ste << "milliseconds (" << ((1.0 * ste) / (1.0 * td.size())) << "msec per contact )";
        elapsedTimeTotal += ste;

        fh.setDetailTypesHint(QList<QContactDetail::DetailType>() << QContactDisplayLabel::Type
                << QContactName::Type << QContactAvatar::Type
                << QContactPhoneNumber::Type << QContactEmailAddress::Type);
        syncTimer.start();
        readContacts = manager.contacts(testingFilter, QList<QContactSortOrder>(), fh);
        ste = syncTimer.elapsed();
        if (readContacts.size() != td.size()) {
            qWarning() << "Invalid retrieval count:" << readContacts.size() << "expecting:" << td.size();
        }
        qDebug() << "    reading all, common details, took" << ste << "milliseconds (" << ((1.0 * ste) / (1.0 * td.size())) << "msec per contact )";
        elapsedTimeTotal += ste;

        fh.setOptimizationHints(QContactFetchHint::NoRelationships);
        fh.setDetailTypesHint(QList<QContactDetail::DetailType>());
        syncTimer.start();
        readContacts = manager.contacts(testingFilter, QList<QContactSortOrder>(), fh);
        ste = syncTimer.elapsed();
        if (readContacts.size() != td.size()) {
            qWarning() << "Invalid retrieval count:" << readContacts.size() << "expecting:" << td.size();
        }
        qDebug() << "    reading all, no relationships, took" << ste << "milliseconds (" << ((1.0 * ste) / (1.0 * td.size())) << "msec per contact )";
        elapsedTimeTotal += ste;

        fh.setDetailTypesHint(QList<QContactDetail::DetailType>() << QContactDisplayLabel::Type
                << QContactName::Type << QContactAvatar::Type);
        syncTimer.start();
        readContacts = manager.contacts(testingFilter, QList<QContactSortOrder>(), fh);
        ste = syncTimer.elapsed();
        if (readContacts.size() != td.size()) {
            qWarning() << "Invalid retrieval count:" << readContacts.size() << "expecting:" << td.size();
        }
        qDebug() << "    reading all, display details + no rels, took" << ste << "milliseconds (" << ((1.0 * ste) / (1.0 * td.size())) << "msec per contact )";
        elapsedTimeTotal += ste;

        // Read the contacts, selected by ID
        QList<QContactId> idsToRetrieve;
        for (int j = 0; j < td.size(); ++j) {
            idsToRetrieve.append(retrievalId(td.at(j)));
        }

        syncTimer.start();
        readContacts = manager.contacts(idsToRetrieve, fh, 0);
        ste = syncTimer.elapsed();
        if (readContacts.size() != td.size()) {
            qWarning() << "Invalid retrieval count:" << readContacts.size() << "expecting:" << td.size();
        }
        qDebug() << "    reading all by IDs, display details + no rels, took" << ste << "milliseconds (" << ((1.0 * ste) / (1.0 * td.size())) << "msec per contact )";
        elapsedTimeTotal += ste;

        // Read the same set using ID filtering
        QContactIdFilter idFilter;
        idFilter.setIds(idsToRetrieve);

        syncTimer.start();
        readContacts = manager.contacts(idFilter, QList<QContactSortOrder>(), fh);
        ste = syncTimer.elapsed();
        if (readContacts.size() != td.size()) {
            qWarning() << "Invalid retrieval count:" << readContacts.size() << "expecting:" << td.size();
        }
        qDebug() << "    reading all by ID filter, display details + no rels, took" << ste << "milliseconds (" << ((1.0 * ste) / (1.0 * td.size())) << "msec per contact )";
        elapsedTimeTotal += ste;

        // Read the same set, but filter everything out using syncTarget
        QContactDetailFilter aggregateFilter;
        aggregateFilter.setDetailType(QContactSyncTarget::Type, QContactSyncTarget::FieldSyncTarget);
        aggregateFilter.setValue(QString::fromLatin1("aggregate"));

        syncTimer.start();
        readContacts = manager.contacts(idFilter & aggregateFilter, QList<QContactSortOrder>(), fh);
        ste = syncTimer.elapsed();
        if (readContacts.size() != 0) {
            qWarning() << "Invalid retrieval count:" << readContacts.size() << "expecting:" << 0;
        }
        qDebug() << "    reading all by ID filter & aggregate, display details + no rels, took" << ste << "milliseconds (" << ((1.0 * ste) / (1.0 * td.size())) << "msec per contact )";
        elapsedTimeTotal += ste;

        QContactDetailFilter firstNameStartsA;
        firstNameStartsA.setDetailType(QContactName::Type, QContactName::FieldFirstName);
        firstNameStartsA.setValue("A");
        firstNameStartsA.setMatchFlags(QContactDetailFilter::MatchStartsWith);
        QContactDetailFilter firstNameStartsZ;
        firstNameStartsZ.setDetailType(QContactName::Type, QContactName::FieldFirstName);
        firstNameStartsZ.setValue("Z");
        firstNameStartsZ.setMatchFlags(QContactDetailFilter::MatchStartsWith);
        fh.setDetailTypesHint(QList<QContactDetail::DetailType>());
        syncTimer.start();
        readContacts = manager.contacts(firstNameStartsA | firstNameStartsZ, QList<QContactSortOrder>(), fh);
        ste = syncTimer.elapsed();
        qDebug() << "    reading filtered (" << readContacts.size() << "), no relationships, took" << ste << "milliseconds (" << ((1.0 * ste) / (1.0 * td.size())) << "msec per contact )";
        elapsedTimeTotal += ste;

        QList<QContactId> idsToRemove;
        for (int j = 0; j < td.size(); ++j) {
            idsToRemove.append(retrievalId(td.at(j)));
        }

        syncTimer.start();
        manager.removeContacts(idsToRemove);
        ste = syncTimer.elapsed();
        qDebug() << "    removing test data took" << ste << "milliseconds (" << ((1.0 * ste) / (1.0 * td.size())) << "msec per contact )";
        elapsedTimeTotal += ste;
    }

    // these tests are slightly different to those above.  They operate on much smaller
    // batches, but occur after the database has already been prefilled with some data.
    QList<int> smallerNbrContacts;
    if (quickMode) {
        smallerNbrContacts << 20;
    } else {
        smallerNbrContacts << 1 << 2 << 5 << 10 << 20 << 50;
    }
    QList<QList<QContact> > smallerTestData;
    qDebug() << "\n\nGenerating smaller test data for prefilled timings...";
    for (int i = 0; i < smallerNbrContacts.size(); ++i) {
        int howMany = smallerNbrContacts.at(i);
        QList<QContact> newTestData;
        newTestData.reserve(howMany);

        for (int j = 0; j < howMany; ++j) {
            newTestData.append(generateContact());
        }

        smallerTestData.append(newTestData);
    }

    // prefill the database
    QList<QContact> prefillData;
    for (int i = 0; i < testData.size() && testData.at(i).size() < 1001; ++i) {
        prefillData = testData.at(i);
    }
    qDebug() << "Prefilling database with" << prefillData.size() << "contacts... this will take a while...";
    manager.saveContacts(&prefillData);
    qDebug() << "Now performing timings (shouldn't get aggregated)...";
    for (int i = 0; i < smallerTestData.size(); ++i) {
        QList<QContact> td = smallerTestData.at(i);
        qint64 ste = 0;
        qDebug() << "Performing tests for" << td.size() << "contacts:";

        syncTimer.start();
        manager.saveContacts(&td);
        ste = syncTimer.elapsed();
        qDebug() << "    saving took" << ste << "milliseconds (" << ((1.0 * ste) / (1.0 * td.size())) << "msec per contact )";
        elapsedTimeTotal += ste;

        QContactFetchHint fh;
        syncTimer.start();
        QList<QContact> readContacts = manager.contacts(QContactFilter(), QList<QContactSortOrder>(), fh);
        ste = syncTimer.elapsed();
        qDebug() << "    reading all (" << readContacts.size() << "), all details, took" << ste << "milliseconds";
        elapsedTimeTotal += ste;

        fh.setDetailTypesHint(QList<QContactDetail::DetailType>() << QContactDisplayLabel::Type
                << QContactName::Type << QContactAvatar::Type
                << QContactPhoneNumber::Type << QContactEmailAddress::Type);
        syncTimer.start();
        readContacts = manager.contacts(QContactFilter(), QList<QContactSortOrder>(), fh);
        ste = syncTimer.elapsed();
        qDebug() << "    reading all, common details, took" << ste << "milliseconds";
        elapsedTimeTotal += ste;

        fh.setOptimizationHints(QContactFetchHint::NoRelationships);
        fh.setDetailTypesHint(QList<QContactDetail::DetailType>());
        syncTimer.start();
        readContacts = manager.contacts(QContactFilter(), QList<QContactSortOrder>(), fh);
        ste = syncTimer.elapsed();
        qDebug() << "    reading all, no relationships, took" << ste << "milliseconds";
        elapsedTimeTotal += ste;

        fh.setDetailTypesHint(QList<QContactDetail::DetailType>() << QContactDisplayLabel::Type
                << QContactName::Type << QContactAvatar::Type);
        syncTimer.start();
        readContacts = manager.contacts(QContactFilter(), QList<QContactSortOrder>(), fh);
        ste = syncTimer.elapsed();
        qDebug() << "    reading all, display details + no rels, took" << ste << "milliseconds";
        elapsedTimeTotal += ste;

        QContactDetailFilter firstNameStartsA;
        firstNameStartsA.setDetailType(QContactName::Type, QContactName::FieldFirstName);
        firstNameStartsA.setValue("A");
        firstNameStartsA.setMatchFlags(QContactDetailFilter::MatchStartsWith);
        QContactDetailFilter firstNameStartsZ;
        firstNameStartsZ.setDetailType(QContactName::Type, QContactName::FieldFirstName);
        firstNameStartsZ.setValue("Z");
        firstNameStartsZ.setMatchFlags(QContactDetailFilter::MatchStartsWith);
        fh.setDetailTypesHint(QList<QContactDetail::DetailType>());
        syncTimer.start();
        readContacts = manager.contacts(firstNameStartsA | firstNameStartsZ, QList<QContactSortOrder>(), fh);
        ste = syncTimer.elapsed();
        qDebug() << "    reading filtered (" << readContacts.size() << "), no relationships, took" << ste << "milliseconds";
        elapsedTimeTotal += ste;

        QList<QContactId> idsToRemove;
        for (int j = 0; j < td.size(); ++j) {
            idsToRemove.append(retrievalId(td.at(j)));
        }

        syncTimer.start();
        manager.removeContacts(idsToRemove);
        ste = syncTimer.elapsed();
        qDebug() << "    removing test data took" << ste << "milliseconds (" << ((1.0 * ste) / (1.0 * td.size())) << "msec per contact )";
        elapsedTimeTotal += ste;
    }

    // The next test is about saving contacts which should get aggregated into others.
    // Aggregation is an expensive operation, so we expect these save operations to take longer.
    qDebug() << "\n\nPerforming aggregation tests";
    QList<QContact> contactsToAggregate;
    for (int i = 0; i < 100; ++i) {
        QContact existingContact = prefillData.at(prefillData.size() - 1 - i);
        QContact contactToAggregate;
        QContactSyncTarget newSyncTarget;
        newSyncTarget.setSyncTarget(QString(QLatin1String("fetchtimes-aggregation")));
        QContactName aggName = existingContact.detail<QContactName>(); // ensures it'll get aggregated
        QContactOnlineAccount newOnlineAcct; // new data, which should get promoted up etc.
        newOnlineAcct.setAccountUri(QString(QLatin1String("test-aggregation-%1@fetchtimes")).arg(i));
        contactToAggregate.saveDetail(&newSyncTarget);
        contactToAggregate.saveDetail(&aggName);
        contactToAggregate.saveDetail(&newOnlineAcct);
        contactsToAggregate.append(contactToAggregate);
    }

    syncTimer.start();
    manager.saveContacts(&contactsToAggregate);
    qint64 aggregationElapsed = syncTimer.elapsed();
    int totalAggregatesInDatabase = manager.contactIds().count();
    qDebug() << "Average time for aggregation of" << contactsToAggregate.size() << "contacts (with" << totalAggregatesInDatabase << "existing in database):" << aggregationElapsed
             << "milliseconds (" << ((1.0 * aggregationElapsed) / (1.0 * contactsToAggregate.size())) << " msec per aggregated contact )";
    elapsedTimeTotal += aggregationElapsed;

    // Now perform the test again, this time with more aggregates, to test nonlinearity.
    contactsToAggregate.clear();
    const int high = prefillData.size() / 2, low = high / 2;
    for (int i = low; i < high; ++i) {
        QContact existingContact = prefillData.at(prefillData.size() - 1 - i);
        QContact contactToAggregate;
        QContactSyncTarget newSyncTarget;
        newSyncTarget.setSyncTarget(QString(QLatin1String("fetchtimes-aggregation")));
        QContactName aggName = existingContact.detail<QContactName>(); // ensures it'll get aggregated
        QContactOnlineAccount newOnlineAcct; // new data, which should get promoted up etc.
        newOnlineAcct.setAccountUri(QString(QLatin1String("test-aggregation-%1@fetchtimes")).arg(i));
        contactToAggregate.saveDetail(&newSyncTarget);
        contactToAggregate.saveDetail(&aggName);
        contactToAggregate.saveDetail(&newOnlineAcct);
        contactsToAggregate.append(contactToAggregate);
    }

    syncTimer.start();
    manager.saveContacts(&contactsToAggregate);
    aggregationElapsed = syncTimer.elapsed();
    totalAggregatesInDatabase = manager.contactIds().count();
    qDebug() << "Average time for aggregation of" << contactsToAggregate.size() << "contacts (with" << totalAggregatesInDatabase << "existing in database):" << aggregationElapsed
             << "milliseconds (" << ((1.0 * aggregationElapsed) / (1.0 * contactsToAggregate.size())) << " msec per aggregated contact )";
    elapsedTimeTotal += aggregationElapsed;

    QContactDetailFilter allSyncTargets;
    allSyncTargets.setDetailType(QContactSyncTarget::Type, QContactSyncTarget::FieldSyncTarget);
    totalAggregatesInDatabase = manager.contactIds().count();
    int totalContactsInDatabase = manager.contactIds(allSyncTargets).count();

    // The next test is about updating existing contacts, amongst a large set.
    // We're especially interested in presence updates, as these are common.
    qDebug() << "\n\nPerforming presence update tests:";

    // in the first presence update test, we update a small number of contacts.
    QStringList presenceAvatars = generateAvatarsList();
    QList<QContact> contactsToUpdate;
    for (int i = 0; i < 10; ++i) {
        contactsToUpdate.append(prefillData.at(prefillData.size() - 1 - i));
    }

    // modify the presence, nickname and avatar of the test data
    for (int j = 0; j < contactsToUpdate.size(); ++j) {
        QString genstr = QString::number(j);
        QContact curr = contactsToUpdate[j];
        QContactPresence cp = curr.detail<QContactPresence>();
        QContactNickname nn = curr.detail<QContactNickname>();
        QContactAvatar av = curr.detail<QContactAvatar>();
        cp.setNickname(genstr);
        cp.setCustomMessage(genstr);
        cp.setTimestamp(QDateTime::currentDateTime());
        cp.setPresenceState(static_cast<QContactPresence::PresenceState>(qrand() % 4));
        nn.setNickname(nn.nickname() + genstr);
        av.setImageUrl(genstr + presenceAvatars.at(qrand() % presenceAvatars.size()));
        curr.saveDetail(&cp);
        curr.saveDetail(&nn);
        curr.saveDetail(&av);
        contactsToUpdate.replace(j, curr);
    }

    // perform a batch save.
    syncTimer.start();
    manager.saveContacts(&contactsToUpdate);
    qint64 presenceElapsed = syncTimer.elapsed();
    totalAggregatesInDatabase = manager.contactIds().count();
    totalContactsInDatabase = manager.contactIds(allSyncTargets).count();
    qDebug() << "    update ( batch of" << contactsToUpdate.size() << ") presence+nick+avatar (with" << totalAggregatesInDatabase << "aggregates and"
             << totalContactsInDatabase << "contacts existing in database, all overlap):" << presenceElapsed
             << "milliseconds (" << ((1.0 * presenceElapsed) / (1.0 * contactsToUpdate.size())) << " msec per updated contact )";
    elapsedTimeTotal += presenceElapsed;

    // in the second presence update test, we update ALL of the contacts
    // This simulates having a large number of contacts from a single source (eg, a social network)
    // where (due to changed connectivity status) presence updates for the entire set become available.
    contactsToUpdate.clear();
    QDateTime timestamp = QDateTime::currentDateTime();
    for (int j = 0; j < prefillData.size(); ++j) {
        QContact curr = prefillData.at(j);
        QString genstr = QString::number(j) + "2";
        QContactPresence cp = curr.detail<QContactPresence>();
        QContactNickname nn = curr.detail<QContactNickname>();
        QContactAvatar av = curr.detail<QContactAvatar>();
        cp.setNickname(genstr);
        cp.setCustomMessage(genstr);
        cp.setTimestamp(timestamp);
        cp.setPresenceState(static_cast<QContactPresence::PresenceState>((qrand() % 4) + 1));
        nn.setNickname(nn.nickname() + genstr);
        av.setImageUrl(genstr + presenceAvatars.at(qrand() % presenceAvatars.size()));
        curr.saveDetail(&cp);
        curr.saveDetail(&nn);
        curr.saveDetail(&av);
        contactsToUpdate.append(curr);
    }

    // perform a batch save.
    syncTimer.start();
    manager.saveContacts(&contactsToUpdate);
    presenceElapsed = syncTimer.elapsed();
    totalAggregatesInDatabase = manager.contactIds().count();
    totalContactsInDatabase = manager.contactIds(allSyncTargets).count();
    qDebug() << "    update ( batch of" << contactsToUpdate.size() << ") presence+nick+avatar (with" << totalAggregatesInDatabase << "aggregates and"
             << totalContactsInDatabase << "contacts existing in database, all overlap):" << presenceElapsed
             << "milliseconds (" << ((1.0 * presenceElapsed) / (1.0 * contactsToUpdate.size())) << " msec per updated contact )";
    elapsedTimeTotal += presenceElapsed;

    // the third presence update test is identical to the previous, but with 2000 prefilled contacts in database.
    qDebug() << "    Adding more prefill data, please wait...";
    QList<QContact> morePrefillData;
    for (int i = 0; i < contactsToUpdate.size(); ++i) {
        morePrefillData.append(generateContact(QString::fromLatin1("testing")));
    }
    manager.saveContacts(&morePrefillData);

    // now do the updates and save.
    contactsToUpdate.clear();
    timestamp = QDateTime::currentDateTime();
    for (int j = 0; j < prefillData.size(); ++j) {
        QContact curr = prefillData.at(j);
        QString genstr = QString::number(j) + "3";
        QContactPresence cp = curr.detail<QContactPresence>();
        QContactNickname nn = curr.detail<QContactNickname>();
        QContactAvatar av = curr.detail<QContactAvatar>();
        cp.setNickname(genstr);
        cp.setCustomMessage(genstr);
        cp.setTimestamp(timestamp);
        cp.setPresenceState(static_cast<QContactPresence::PresenceState>((qrand() % 4) + 1));
        nn.setNickname(nn.nickname() + genstr);
        av.setImageUrl(genstr + presenceAvatars.at(qrand() % presenceAvatars.size()));
        curr.saveDetail(&cp);
        curr.saveDetail(&nn);
        curr.saveDetail(&av);
        contactsToUpdate.append(curr);
    }
    for (int j = 0; j < morePrefillData.size(); ++j) {
        QContact curr = morePrefillData.at(j);
        QString genstr = QString::number(j) + "3";
        QContactPresence cp = curr.detail<QContactPresence>();
        QContactNickname nn = curr.detail<QContactNickname>();
        QContactAvatar av = curr.detail<QContactAvatar>();
        cp.setNickname(genstr);
        cp.setCustomMessage(genstr);
        cp.setTimestamp(timestamp);
        cp.setPresenceState(static_cast<QContactPresence::PresenceState>((qrand() % 4) + 1));
        nn.setNickname(nn.nickname() + genstr);
        av.setImageUrl(genstr + presenceAvatars.at(qrand() % presenceAvatars.size()));
        curr.saveDetail(&cp);
        curr.saveDetail(&nn);
        curr.saveDetail(&av);
        contactsToUpdate.append(curr);
    }

    // perform a batch save.
    syncTimer.start();
    manager.saveContacts(&contactsToUpdate);
    presenceElapsed = syncTimer.elapsed();
    totalAggregatesInDatabase = manager.contactIds().count();
    totalContactsInDatabase = manager.contactIds(allSyncTargets).count();
    qDebug() << "    update ( batch of" << contactsToUpdate.size() << ") presence+nick+avatar (with" << totalAggregatesInDatabase << "aggregates and"
             << totalContactsInDatabase << "contacts existing in database, all overlap):" << presenceElapsed
             << "milliseconds (" << ((1.0 * presenceElapsed) / (1.0 * contactsToUpdate.size())) << " msec per updated contact )";
    elapsedTimeTotal += presenceElapsed;

    // clean up the "more prefill data"
    qDebug() << "    cleaning up extra prefill data, please wait...";
    QList<QContactId> morePrefillIds;
    for (int j = 0; j < morePrefillData.size(); ++j) {
        morePrefillIds.append(retrievalId(morePrefillData.at(j)));
    }

    manager.removeContacts(morePrefillIds);

    // the fourth presence update test checks update time for non-overlapping sets of data.
    qDebug() << "    generating non-overlapping / aggregated prefill data, please wait...";
    morePrefillData.clear();
    for (int i = 0; i < prefillData.size(); ++i) {
        morePrefillData.append(generateContact("test-presence-4", false)); // false = don't aggregate.
    }
    manager.saveContacts(&morePrefillData);

    // now do the update
    contactsToUpdate.clear();
    timestamp = QDateTime::currentDateTime();
    for (int j = 0; j < morePrefillData.size(); ++j) {
        QContact curr = morePrefillData.at(j);
        QString genstr = QString::number(j) + "4";
        QContactPresence cp = curr.detail<QContactPresence>();
        QContactNickname nn = curr.detail<QContactNickname>();
        QContactAvatar av = curr.detail<QContactAvatar>();
        cp.setNickname(genstr);
        cp.setCustomMessage(genstr);
        cp.setTimestamp(timestamp);
        cp.setPresenceState(static_cast<QContactPresence::PresenceState>((qrand() % 4) + 1));
        nn.setNickname(nn.nickname() + genstr);
        av.setImageUrl(genstr + presenceAvatars.at(qrand() % presenceAvatars.size()));
        curr.saveDetail(&cp);
        curr.saveDetail(&nn);
        curr.saveDetail(&av);
        contactsToUpdate.append(curr);
    }

    // perform a batch save.
    syncTimer.start();
    manager.saveContacts(&contactsToUpdate);
    presenceElapsed = syncTimer.elapsed();
    totalAggregatesInDatabase = manager.contactIds().count();
    totalContactsInDatabase = manager.contactIds(allSyncTargets).count();
    qDebug() << "    update ( batch of" << contactsToUpdate.size() << ") presence+nick+avatar (with" << totalAggregatesInDatabase << "aggregates and"
             << totalContactsInDatabase << "contacts existing in database, no overlap):" << presenceElapsed
             << "milliseconds (" << ((1.0 * presenceElapsed) / (1.0 * contactsToUpdate.size())) << " msec per updated contact )";
    elapsedTimeTotal += presenceElapsed;

    // clean up the "more prefill data"
    qDebug() << "    cleaning up extra prefill data, please wait...";
    morePrefillIds.clear();
    for (int j = 0; j < morePrefillData.size(); ++j) {
        morePrefillIds.append(morePrefillData.at(j).id());
    }
    manager.removeContacts(morePrefillIds);

    // the fifth presence update test is similar to the above except that half of
    // the extra contacts have a (high) chance of being aggregated into an existing contact.
    // So, database should have 2000 constituents, 1000 from "local", 1000 from "test-presence-5"
    // with 1500 aggregates (about 500 of test-presence-5 contacts will share an aggregate with
    // a local contact).  TODO: check what happens if multiple aggregates for local contacts
    // could possibly match a given test-presence-5 contact (which is possible, since the backend
    // never aggregates two contacts from the same sync source...)
    qDebug() << "    generating partially-overlapping / aggregated prefill data, please wait...";
    morePrefillData.clear();
    for (int i = 0; i < prefillData.size(); ++i) {
        if (i < (prefillData.size() / 2)) {
            morePrefillData.append(generateContact("test-presence-5", false)); // false = don't aggregate.
        } else {
            morePrefillData.append(generateContact("test-presence-5", true));  // true = possibly aggregate.
        }
    }
    manager.saveContacts(&morePrefillData);

    // now do the update
    contactsToUpdate.clear();
    timestamp = QDateTime::currentDateTime();
    for (int j = 0; j < morePrefillData.size(); ++j) {
        QContact curr = morePrefillData.at(j);
        QString genstr = QString::number(j) + "5";
        QContactPresence cp = curr.detail<QContactPresence>();
        QContactNickname nn = curr.detail<QContactNickname>();
        QContactAvatar av = curr.detail<QContactAvatar>();
        cp.setNickname(genstr);
        cp.setCustomMessage(genstr);
        cp.setTimestamp(timestamp);
        cp.setPresenceState(static_cast<QContactPresence::PresenceState>((qrand() % 4) + 1));
        nn.setNickname(nn.nickname() + genstr);
        av.setImageUrl(genstr + presenceAvatars.at(qrand() % presenceAvatars.size()));
        curr.saveDetail(&cp);
        curr.saveDetail(&nn);
        curr.saveDetail(&av);
        contactsToUpdate.append(curr);
    }

    // perform a batch save.
    syncTimer.start();
    manager.saveContacts(&contactsToUpdate);
    presenceElapsed = syncTimer.elapsed();
    totalAggregatesInDatabase = manager.contactIds().count();
    totalContactsInDatabase = manager.contactIds(allSyncTargets).count();
    qDebug() << "    update ( batch of" << contactsToUpdate.size() << ") presence+nick+avatar (with" << totalAggregatesInDatabase << "aggregates and"
             << totalContactsInDatabase << "contacts existing in database, partial overlap):" << presenceElapsed
             << "milliseconds (" << ((1.0 * presenceElapsed) / (1.0 * contactsToUpdate.size())) << " msec per updated contact )";
    elapsedTimeTotal += presenceElapsed;

    // the sixth presence update test is identical to the fifth test, except that we ONLY
    // update the presence status (not nickname or avatar).
    morePrefillData = contactsToUpdate;
    contactsToUpdate.clear();
    for (int j = 0; j < morePrefillData.size(); ++j) {
        QContact curr = morePrefillData.at(j);
        QContactPresence cp = curr.detail<QContactPresence>();
        cp.setPresenceState(static_cast<QContactPresence::PresenceState>((qrand() % 4) + 1));
        curr.saveDetail(&cp);
        contactsToUpdate.append(curr);
    }

    // perform a batch save.
    syncTimer.start();
    manager.saveContacts(&contactsToUpdate);
    presenceElapsed = syncTimer.elapsed();
    totalAggregatesInDatabase = manager.contactIds().count();
    totalContactsInDatabase = manager.contactIds(allSyncTargets).count();
    qDebug() << "    update ( batch of" << contactsToUpdate.size() << ") presence only (with" << totalAggregatesInDatabase << "aggregates and"
             << totalContactsInDatabase << "contacts existing in database, partial overlap):" << presenceElapsed
             << "milliseconds (" << ((1.0 * presenceElapsed) / (1.0 * contactsToUpdate.size())) << " msec per updated contact )";
    elapsedTimeTotal += presenceElapsed;

    // the seventh presence update test is identical to the 6th test, except that
    // we also pass a "detail type mask" to the update.  This allows the backend
    // to perform optimisation based upon which details are modified.
    morePrefillData = contactsToUpdate;
    contactsToUpdate.clear();
    for (int j = 0; j < morePrefillData.size(); ++j) {
        QContact curr = morePrefillData.at(j);
        QContactPresence cp = curr.detail<QContactPresence>();
        cp.setPresenceState(static_cast<QContactPresence::PresenceState>((qrand() % 4) + 1));
        curr.saveDetail(&cp);
        contactsToUpdate.append(curr);
    }

    // perform a batch save.
    QList<QContactDetail::DetailType> typeMask;
    typeMask << QContactDetail::TypePresence;
    syncTimer.start();
    manager.saveContacts(&contactsToUpdate, typeMask);
    presenceElapsed = syncTimer.elapsed();
    totalAggregatesInDatabase = manager.contactIds().count();
    totalContactsInDatabase = manager.contactIds(allSyncTargets).count();
    qDebug() << "    update ( batch of" << contactsToUpdate.size() << ") masked presence only (with" << totalAggregatesInDatabase << "aggregates and"
             << totalContactsInDatabase << "contacts existing in database, partial overlap):" << presenceElapsed
             << "milliseconds (" << ((1.0 * presenceElapsed) / (1.0 * contactsToUpdate.size())) << " msec per updated contact )";
    elapsedTimeTotal += presenceElapsed;

    // clean up the "more prefill data"
    qDebug() << "    cleaning up extra prefill data, please wait...";
    morePrefillIds.clear();
    for (int j = 0; j < morePrefillData.size(); ++j) {
        morePrefillIds.append(morePrefillData.at(j).id());
    }
    manager.removeContacts(morePrefillIds);

    // Now performing a "simple" presence update: generate 500 contacts and update 200 presences.
    // Each contact has 1 aggregate and 1 constituent.
    manager.removeContacts(manager.contactIds());
    totalAggregatesInDatabase = manager.contactIds().count();
    totalContactsInDatabase = manager.contactIds(allSyncTargets).count();
    qDebug() << "currently have:" << totalAggregatesInDatabase << "aggregates and" << totalContactsInDatabase << "altogether in database.";
    qDebug() << "\n\nGenerating contacts for simple presence update test.";
    QList<QContact> simplePresenceContacts;
    QStringList spFirstNames = generateNonOverlappingFirstNamesList();
    QStringList spLastNames = generateLastNamesList();
    QStringList spPhones = generatePhoneNumbersList();
    QStringList spEmails = generateEmailProvidersList();
    int spTotalCount = 0;
    Q_FOREACH(const QString &fn, spFirstNames) {
        Q_FOREACH(const QString &ln, spLastNames) {
            QContact spc;
            QContactName spcn;
            spcn.setFirstName(fn);
            spcn.setLastName(ln);
            spc.saveDetail(&spcn);
            QContactPhoneNumber spcp;
            spcp.setNumber(spPhones[rand() % spPhones.size()]);
            spc.saveDetail(&spcp);
            QContactEmailAddress spce;
            spce.setEmailAddress(QStringLiteral("%1.%2@%3").arg(fn).arg(ln).arg(spEmails[rand() % spEmails.size()]));
            spc.saveDetail(&spce);
            QContactSyncTarget spcst;
            spcst.setSyncTarget("simplepresence");
            spc.saveDetail(&spcst);
            simplePresenceContacts.append(spc);
            spTotalCount++;
            if (spTotalCount >= 500) {
                break;
            }
        }
        if (spTotalCount >= 500) {
            break;
        }
    }
    syncTimer.start();
    manager.saveContacts(&simplePresenceContacts);
    presenceElapsed = syncTimer.elapsed();
    qDebug() << "    Adding" << simplePresenceContacts.size() << "contacts took:" << presenceElapsed << "msec"
             << "(" << ((1.0 * presenceElapsed) / (1.0 * simplePresenceContacts.size())) << " msec per updated contact )";
    elapsedTimeTotal += presenceElapsed;

    // now add presence for 200 of those contacts, and save them.
    QList<QContact> updatedSimplePresenceContacts;
    for (int j = 101; j < 301; ++j) {
        QContact curr = simplePresenceContacts.at(j);
        QContactPresence cp = curr.detail<QContactPresence>();
        cp.setPresenceState(static_cast<QContactPresence::PresenceState>((j%3)+1));
        curr.saveDetail(&cp);
        updatedSimplePresenceContacts.append(curr);
    }

    // perform a batch save passing a detail type mask to allow optimisation.
    typeMask.clear();
    typeMask << QContactDetail::TypePresence;
    syncTimer.start();
    manager.saveContacts(&updatedSimplePresenceContacts, typeMask);
    presenceElapsed = syncTimer.elapsed();
    qDebug() << "    Adding presence to" << updatedSimplePresenceContacts.size()
             << "of those" << simplePresenceContacts.size() << "contacts took:" << presenceElapsed << "msec"
             << "(" << ((1.0 * presenceElapsed) / (1.0 * updatedSimplePresenceContacts.size())) << " msec per updated contact )";
    elapsedTimeTotal += presenceElapsed;

    // now do a similar modification.  This will result in update (rather than creation) of presence for each contact.
    for (int j = 0; j < updatedSimplePresenceContacts.size(); ++j) {
        QContact curr = updatedSimplePresenceContacts[j];
        QContactPresence cp = curr.detail<QContactPresence>();
        cp.setPresenceState(static_cast<QContactPresence::PresenceState>((j%3)+1));
        curr.saveDetail(&cp);
        updatedSimplePresenceContacts.replace(j, curr);
    }

    // perform a batch save passing a detail type mask to allow optimisation.
    typeMask.clear();
    typeMask << QContactDetail::TypePresence;
    syncTimer.start();
    manager.saveContacts(&updatedSimplePresenceContacts, typeMask);
    presenceElapsed = syncTimer.elapsed();
    qDebug() << "    Updating presence of those same" << updatedSimplePresenceContacts.size()
             << "of" << simplePresenceContacts.size() << "contacts took:" << presenceElapsed << "msec"
             << "(" << ((1.0 * presenceElapsed) / (1.0 * updatedSimplePresenceContacts.size())) << " msec per updated contact )";
    elapsedTimeTotal += presenceElapsed;

    // clean up the added presence contacts.
    QList<QContactId> spCleanup;
    Q_FOREACH (const QContact &c, simplePresenceContacts) {
        spCleanup.append(c.id());
    }
    manager.removeContacts(spCleanup);

    qDebug() << "\n\nCumulative elapsed time:" << elapsedTimeTotal << "milliseconds";
    return 0;
}
