package dialogs;

import data.Standards;
import taskglacier.MainFrame;

import javax.swing.*;
import java.awt.*;
import java.io.IOException;
import java.util.prefs.Preferences;

public class ConnectToServer extends JDialog {
    public ConnectToServer(MainFrame mainFrame) {
        setLayout(new GridBagLayout());
        setTitle("Connect");
        setModal(true);
        
        GridBagConstraints gbc = new GridBagConstraints();

        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        gbc.gridx = 0;
        gbc.gridy = 0;

        JPanel ipPanel = new JPanel(new FlowLayout());
        ipPanel.add(new JLabel("IP Address: "));

        JTextField IP = new JTextField();
        ipPanel.add(IP);

        add(ipPanel, gbc);
        gbc.gridy++;

        JPanel portPanel = new JPanel(new FlowLayout());
        portPanel.add(new JLabel("Port: "));

        JTextField port = new JTextField();
        portPanel.add(port);

        add(portPanel, gbc);
        gbc.gridy++;

        JCheckBox rememberServer = new JCheckBox("Remember Server");
        add(rememberServer, gbc);
        gbc.gridy++;

        JButton connectButton = new JButton("Connect");
        add(connectButton, gbc);

        Preferences preferences;

        if (System.getenv("TASK_GLACIER_DEV_INSTANCE") != null) {
            preferences = Preferences.userNodeForPackage(ConnectToServer.class);
        }
        else {
            preferences = Preferences.userNodeForPackage(MainFrame.class);
        }

        IP.setText(preferences.get("IP-Address", "127.0.0.1"));
        port.setText(String.valueOf(preferences.getInt("Port", 5000)));
        rememberServer.setSelected(preferences.getBoolean("Remember-Server", false));

        connectButton.addActionListener(e -> {
            try {
                mainFrame.createConnection(IP.getText(), Integer.parseInt(port.getText()));

                preferences.putBoolean("Remember-Server", rememberServer.isSelected());

                if (rememberServer.isSelected()) {
                    preferences.put("IP-Address", IP.getText());
                    preferences.putInt("Port", Integer.parseInt(port.getText()));
                }
                else {
                    preferences.remove("IP-Address");
                    preferences.remove("Port");
                }
            } catch (IOException ex) {
                JOptionPane.showMessageDialog(ConnectToServer.this, "Connection to " + IP.getText() + ":" + port.getText() + " Failed", "Failed to Connect", JOptionPane.ERROR_MESSAGE);
            }

            ConnectToServer.this.dispose();
        });
        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
