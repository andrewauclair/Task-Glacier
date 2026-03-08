if not defined RELEASE_VERSION (
    for /f "tokens=*" %%i in ('git describe --tags --always') do set RELEASE_VERSION=%%i
)

call gradlew build

jpackage --name "Task Glacier" --app-version "%RELEASE_VERSION%" --icon app-icon-64.ico -i build/libs --main-class taskglacier.MainFrame --main-jar task-glacier-ui.jar --win-menu --win-dir-chooser
