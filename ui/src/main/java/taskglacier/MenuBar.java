package taskglacier;

import dialogs.AddModifyTask;
import dialogs.BugzillaConfiguration;
import dialogs.ConnectToServer;
import dialogs.TimeCategories;
import io.github.andrewauclair.moderndocking.app.DockingState;
import io.github.andrewauclair.moderndocking.layouts.DockingLayouts;
import packets.BugzillaRefresh;
import packets.RequestDailyReport;
import packets.RequestID;

import javax.swing.*;

public class MenuBar extends JMenuBar {
    private final JMenuItem add;

    private final JMenuItem connect = new JMenuItem("Connect...");
    private final JMenuItem disconnect = new JMenuItem("Disconnect");
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
        add.addActionListener(e -> new AddModifyTask(mainFrame, 0, false).setVisible(true));
        task.add(add);

        JMenuItem timeCategories = new JMenuItem("Time Categories...");
        task.add(timeCategories);

        timeCategories.addActionListener(e -> new TimeCategories(mainFrame).setVisible(true));

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

        JMenuItem requestDailyReport = new JMenuItem("Request Daily Report");
        server.add(requestDailyReport);

        requestDailyReport.addActionListener(e -> {
            RequestDailyReport request = new RequestDailyReport();
            request.requestID = RequestID.nextRequestID();
            request.month = 3;
            request.day = 4;
            request.year = 2025;

            mainFrame.getConnection().sendPacket(request);
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
        bugzilla.setEnabled(true);
    }

    public void disconnected() {
        add.setEnabled(false);
        connect.setEnabled(true);
        disconnect.setEnabled(false);
        bugzilla.setEnabled(false);
    }
}
