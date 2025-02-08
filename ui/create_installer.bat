call gradlew build

jpackage --name "Task Glacier" --app-version 0.0.8 --icon app-icon-64.ico -i build/libs --main-class taskglacier.MainFrame --main-jar task-glacier-ui.jar --win-menu --win-dir-chooser