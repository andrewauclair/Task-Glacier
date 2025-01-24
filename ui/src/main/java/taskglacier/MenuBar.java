package taskglacier;

import dialogs.AddTask;
import dialogs.ConnectToServer;
import packets.RequestConfig;

import javax.swing.*;
import java.io.IOException;

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
        add.addActionListener(e -> new AddTask(mainFrame, mainFrame.output).setVisible(true));
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

        JMenuItem request = new JMenuItem("Request Config");
        request.addActionListener(e -> {
            RequestConfig requestConfig = new RequestConfig();
            try {
                mainFrame.clearTasks();
                requestConfig.writeToStream(mainFrame.output);
            } catch (IOException ex) {
                throw new RuntimeException(ex);
            }
        });
        server.add(request);
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
