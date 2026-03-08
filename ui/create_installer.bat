REM if no release version, set to 0.0.0 to satisfy jpackage
REM jpackage requires semantic versioning and won't work with our git describe version (which will be shown in the UI About dialog)
if not defined RELEASE_VERSION set RELEASE_VERSION=0.0.0

call gradlew build

jpackage --name "Task Glacier" --app-version %RELEASE_VERSION% --icon app-icon-64.ico -i build/libs --main-class taskglacier.MainFrame --main-jar task-glacier-ui.jar --win-menu --win-dir-chooser
