# Юнит-тесты ядра Журнала ТО (расчёт сроков + SQLite). В сборку программы не входят.
# Сборка: qmake && make && ./tst_tojournal   (нужны Qt 5.15 testlib, sql и драйвер QSQLITE)
QT += testlib sql
QT -= gui
CONFIG += console c++14 warn_on
TARGET = tst_tojournal
SOURCES += tst_tojournal.cpp ../tojournalcalc.cpp ../tojournalstore.cpp
HEADERS += ../tojournaltypes.h ../tojournalcalc.h ../tojournalstore.h ../tojournaldefaults.h
