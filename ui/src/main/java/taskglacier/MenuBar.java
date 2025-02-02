package taskglacier;

import dialogs.AddModifyTask;
import dialogs.ConnectToServer;

import javax.swing.*;

public class MenuBar extends JMenuBar {
    private final JMenuItem add;

    private final JMenuItem connect = new JMenuItem("Connect...");
    private final JMenuItem disconnect = new JMenuItem("Disconnect");

    public MenuBar(MainFrame mainFrame) {
        JMenu task = new JMenu("Task");
        add = new JMenuItem("Add");
        add.setEnabled(false);
        add.addActionListener(e -> new AddModifyTask(mainFrame, false).setVisible(true));
        task.add(add);

        add(task);

        JMenu server = new JMenu("Server");

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

        add(server);
    }

    public void connected() {
        add.setEnabled(true);
        connect.setEnabled(false);
        disconnect.setEnabled(true);
    }

    public void disconnected() {
        add.setEnabled(false);
        connect.setEnabled(true);
        disconnect.setEnabled(false);
    }
}
