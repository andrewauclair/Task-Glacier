package dialogs;

import data.Standards;
import packets.BugzillaInfo;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.util.prefs.Preferences;

public class BugzillaConfiguration extends JDialog {
    // TODO configure: server address, api key, and search rules

    public BugzillaConfiguration(MainFrame mainFrame) {
        setLayout(new GridBagLayout());
        setTitle("Bugzilla Configuration");
        setModal(true);

        GridBagConstraints gbc = new GridBagConstraints();

        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        gbc.gridx = 0;
        gbc.gridy = 0;

        JTextField URL = new JTextField();
        add(createLabeledComponent("UR", URL), gbc);
        gbc.gridy++;

        JTextField apiKey = new JTextField();
        add(createLabeledComponent("API Key", apiKey), gbc);
        gbc.gridy++;

        JTextField username = new JTextField();
        add(createLabeledComponent("Username", username), gbc);
        gbc.gridy++;

        JTextField rootTask = new JTextField();
        add(createLabeledComponent("Root Task", rootTask), gbc);
        gbc.gridy++;

        // option to split tasks by a specific bugzilla field
        JTextField groupTasksBy = new JTextField("target_milestone");
        add(createLabeledComponent("Group Tasks By", groupTasksBy), gbc);

        // configure search parameters and task arrangement
        // configure labels
        DefaultTableModel tableModel = new DefaultTableModel(new Object[] { "label", "bugzilla field"}, 0);
        JTable table = new JTable(tableModel);

        add(new JScrollPane(table), gbc);

        JButton save = new JButton("Save");
        add(save, gbc);

        Preferences preferences;

        if (System.getenv("TASK_GLACIER_DEV_INSTANCE") != null) {
            preferences = Preferences.userNodeForPackage(BugzillaConfiguration.class);
        }
        else {
            preferences = Preferences.userNodeForPackage(MainFrame.class);
        }

        URL.setText(preferences.get("Bugzilla-URL", "bugzilla"));
        apiKey.setText(preferences.get("Bugzilla-API-Key", ""));
        username.setText(preferences.get("Bugzilla-Username", ""));

        save.addActionListener(e -> {
            BugzillaInfo info = new BugzillaInfo(URL.getText(), apiKey.getText(), username.getText());
            mainFrame.getConnection().sendPacket(info);

            preferences.put("Bugzilla-URL", URL.getText());
            preferences.put("Bugzilla-API-Key", apiKey.getText());
            preferences.put("Bugzilla-Username", username.getText());

            BugzillaConfiguration.this.dispose();
        });
        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }

    JPanel createLabeledComponent(String name, JComponent component) {
        JPanel panel = new JPanel(new FlowLayout());
        panel.add(new JLabel(name + " "));
        panel.add(component);
        return panel;
    }
}
