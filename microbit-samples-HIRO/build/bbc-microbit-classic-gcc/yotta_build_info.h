#ifndef __YOTTA_BUILD_INFO_H__
#define __YOTTA_BUILD_INFO_H__
// yotta build info, #include YOTTA_BUILD_INFO_HEADER to access
#define YOTTA_BUILD_YEAR 2020 // UTC year
#define YOTTA_BUILD_MONTH 6 // UTC month 1-12
#define YOTTA_BUILD_DAY 25 // UTC day 1-31
#define YOTTA_BUILD_HOUR 19 // UTC hour 0-24
#define YOTTA_BUILD_MINUTE 44 // UTC minute 0-59
#define YOTTA_BUILD_SECOND 4 // UTC second 0-61
#define YOTTA_BUILD_UUID 7115f092-862d-4529-bed4-d0e1cd649f9d // unique random UUID for each build
#define YOTTA_BUILD_VCS_ID b'c3b02564fd8082f3bf80472e679aa2f9b707c7c2' // git or mercurial hash
#define YOTTA_BUILD_VCS_CLEAN 0 // evaluates true if the version control system was clean, otherwise false
#define YOTTA_BUILD_VCS_DESCRIPTION b'v2.1.1' // git describe or mercurial equivalent
#endif // ndef __YOTTA_BUILD_INFO_H__
