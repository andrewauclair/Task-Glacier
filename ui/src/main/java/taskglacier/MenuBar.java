package taskglacier;

import dialogs.AddTask;
import dialogs.ConnectToServer;

import javax.management.remote.JMXConnectorServer;
import javax.swing.*;
import java.awt.*;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.util.Arrays;

public class MenuBar extends JMenuBar {
    private final MainFrame mainFrame;
    private final JMenuItem add;

    private final JMenuItem connect = new JMenuItem("Connect...");
    private final JMenuItem disconnect = new JMenuItem("Disconnect");

    public MenuBar(MainFrame mainFrame) {
        this.mainFrame = mainFrame;
        JMenu task = new JMenu("Task");
        add = new JMenuItem("Add");
        add.setEnabled(false);
        add.addActionListener(e -> new AddTask(mainFrame.output).setVisible(true));
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
        disconnect.setEnabled(true);
    }

    public void disconnected() {
        add.setEnabled(false);
        disconnect.setEnabled(false);
    }
}
