package dialogs;

import data.Standards;
import packets.BugzillaInfo;
import taskglacier.MainFrame;

import javax.swing.*;
import java.awt.*;
import java.util.prefs.Preferences;

public class Bugzilla extends JDialog {
    // TODO configure: server address, api key, and search rules

    public Bugzilla(MainFrame mainFrame) {
        setLayout(new GridBagLayout());
        setTitle("Bugzilla Configuration");
        setModal(true);

        GridBagConstraints gbc = new GridBagConstraints();

        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        gbc.gridx = 0;
        gbc.gridy = 0;

        JPanel urlPanel = new JPanel(new FlowLayout());
        urlPanel.add(new JLabel("URL: "));

        JTextField URL = new JTextField();
        urlPanel.add(URL);

        add(urlPanel, gbc);
        gbc.gridy++;

        JPanel keyPanel = new JPanel(new FlowLayout());
        keyPanel.add(new JLabel("API Key: "));

        JTextField apiKey = new JTextField();
        keyPanel.add(apiKey);

        add(keyPanel, gbc);
        gbc.gridy++;

        JCheckBox rememberServer = new JCheckBox("Remember Bugzilla Server");
        add(rememberServer, gbc);
        gbc.gridy++;

        JButton connectButton = new JButton("Connect");
        add(connectButton, gbc);

        Preferences preferences;

        if (System.getenv("TASK_GLACIER_DEV_INSTANCE") != null) {
            preferences = Preferences.userNodeForPackage(Bugzilla.class);
        }
        else {
            preferences = Preferences.userNodeForPackage(MainFrame.class);
        }

        URL.setText(preferences.get("Bugzilla-URL", "bugzilla"));
        apiKey.setText(preferences.get("Bugzilla-API-Key", ""));
        rememberServer.setSelected(preferences.getBoolean("Remember-Bugzilla-Server", false));

        connectButton.addActionListener(e -> {
            BugzillaInfo info = new BugzillaInfo(URL.getText(), apiKey.getText());
            mainFrame.getConnection().sendPacket(info);

            preferences.putBoolean("Remember-Bugzilla-Server", rememberServer.isSelected());

            if (rememberServer.isSelected()) {
                preferences.put("Bugzilla-URL", URL.getText());
                preferences.put("Bugzilla-API-Key", "");
            }
            else {
                preferences.remove("Bugzilla-URL");
                preferences.remove("Bugzilla-API-Key");
            }

            Bugzilla.this.dispose();
        });
        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
