call gradlew build

jpackage --name "Task Glacier" --module-path .;build/libs/ --module taskglacier/task.glacier.ui --add-modules taskglacier --app-version 0.2.3 --icon app-icon-64.ico -i build/libs --win-menu --win-dir-chooser