package taskglacier;

import dialogs.*;
import io.github.andrewauclair.moderndocking.app.DockingState;
import io.github.andrewauclair.moderndocking.layouts.DockingLayouts;
import packets.BugzillaRefresh;
import packets.RequestID;
import panels.TaskSearch;

import javax.swing.*;
import java.awt.event.ActionEvent;

public class MenuBar extends JMenuBar {
    private final JMenuItem add;

    private final JMenuItem connect = new JMenuItem("Connect...");
    private final JMenuItem disconnect = new JMenuItem("Disconnect");
    private final JMenuItem requestDailyReport = new JMenuItem("Request Daily Report...");
    private final JMenuItem requestWeeklyReport = new JMenuItem("Request Weekly Report...");

    private final JMenu bugzilla = new JMenu("Bugzilla");

    public MenuBar(MainFrame mainFrame) {
        JMenu file = new JMenu("File");
        file.setMnemonic('F');

        JMenuItem hide = new JMenuItem("Hide");
        hide.addActionListener(e -> mainFrame.setVisible(false));
        file.add(hide);

        file.addSeparator();

        JMenuItem exit = new JMenuItem("Exit");
        exit.addActionListener(e -> System.exit(0));
        file.add(exit);

        add(file);

        JMenu task = new JMenu("Task");
        task.setMnemonic('T');

        add = new JMenuItem("Add...");
        add.setEnabled(false);
        add.addActionListener(e -> new AddModifyTask(mainFrame, mainFrame, 0, false).setVisible(true));
        task.add(add);

        JMenuItem timeEntry = new JMenuItem("Time Entry Configuration...");
        task.add(timeEntry);

        timeEntry.addActionListener(e -> new TimeEntryConfiguration(mainFrame).setVisible(true));

        JMenuItem search = new JMenuItem("Search...");
        search.addActionListener(e -> new SearchDialog(mainFrame).setVisible(true));
        task.add(search);

        add(task);


        JMenu server = new JMenu("Server");
        server.setMnemonic('S');

        connect.addActionListener(e -> {
            ConnectToServer connectToServer = new ConnectToServer(mainFrame);
            connectToServer.setVisible(true);
        });

        disconnect.addActionListener(e -> {
            mainFrame.disconnect();
        });
        disconnect.setEnabled(false);

        server.add(connect);
        server.add(disconnect);
        server.addSeparator();

        server.add(requestDailyReport);

        requestDailyReport.addActionListener(e -> {
            RequestDailyReportDialog dialog = new RequestDailyReportDialog(mainFrame);
            dialog.setVisible(true);
        });

        server.add(requestWeeklyReport);

        requestWeeklyReport.addActionListener(e -> {
            RequestWeeklyReportDialog dialog = new RequestWeeklyReportDialog(mainFrame);
            dialog.setVisible(true);
        });

        add(server);

        bugzilla.setMnemonic('B');
        bugzilla.setEnabled(false);

        JMenuItem configure = new JMenuItem("Configure...");
        configure.addActionListener(e -> {
            BugzillaConfiguration config = new BugzillaConfiguration(mainFrame);
            config.setVisible(true);
        });
        bugzilla.add(configure);

        JMenuItem refresh = new JMenuItem("Refresh");
        refresh.addActionListener(e -> {
            BugzillaRefresh packet = new BugzillaRefresh(RequestID.nextRequestID());
            mainFrame.getConnection().sendPacket(packet);
        });
        bugzilla.add(refresh);
        add(bugzilla);

        JMenu window = new JMenu("Window");
        JMenuItem restoreDefaultLayout = new JMenuItem("Restore Default Layout");
        restoreDefaultLayout.addActionListener(e -> {
            DockingState.restoreApplicationLayout(DockingLayouts.getLayout("default"));
        });

        window.add(restoreDefaultLayout);
        add(window);
    }

    public void connected() {
        add.setEnabled(true);
        connect.setEnabled(false);
        disconnect.setEnabled(true);
        requestDailyReport.setEnabled(true);
        bugzilla.setEnabled(true);
    }

    public void disconnected() {
        add.setEnabled(false);
        connect.setEnabled(true);
        disconnect.setEnabled(false);
        requestDailyReport.setEnabled(false);
        bugzilla.setEnabled(false);
    }
}
