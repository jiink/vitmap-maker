@echo off
rem Opens the Vitmap editor in your default browser.
rem A temporary local server runs just long enough for the page to load.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\serve-once.ps1"
