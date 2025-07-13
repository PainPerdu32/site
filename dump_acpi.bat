@echo off
echo Dumping ACPI tables to acpi.dat...
acpi-view.exe -b > acpi.dat
echo Done.
