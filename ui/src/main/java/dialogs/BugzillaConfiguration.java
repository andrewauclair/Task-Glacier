package dialogs;

import data.Standards;
import packets.BugzillaInfo;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.util.HashMap;
import java.util.Map;
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

        // TODO currently we'll do all searching by 'assigned_to' in the bugzilla search. In the future we should support searching by other fields as well, such as component.
        JTextField username = new JTextField();
        add(createLabeledComponent("Username", username), gbc);
        gbc.gridy++;

        JTextField rootTask = new JTextField();
        add(createLabeledComponent("Root Task", rootTask), gbc);
        gbc.gridy++;

        // option to split tasks by a specific bugzilla field
        JTextField groupTasksBy = new JTextField("target_milestone");
        add(createLabeledComponent("Group Tasks By", groupTasksBy), gbc);
        gbc.gridy++;

        // configure search parameters and task arrangement
        // configure labels
        DefaultTableModel tableModel = new DefaultTableModel(new Object[] { "Label", "Bugzilla Field"}, 0);
        JTable table = new JTable(tableModel);

        tableModel.addRow(new Object[] { "Priority", "priority" });
        tableModel.addRow(new Object[] { "Status", "status" });
        tableModel.addRow(new Object[] { "Resolution", "resolution" });
        tableModel.addRow(new Object[] { "Severity", "severity" });
        tableModel.addRow(new Object[] { "Target", "target_milestone" });
        tableModel.addRow(new Object[] { "Component", "component" });

        add(new JScrollPane(table), gbc);
        gbc.gridy++;

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
            info.setGroupTasksBy(groupTasksBy.getText());
            info.setRootTaskID(Integer.parseInt(rootTask.getText()));
            Map<String, String> labelToField = new HashMap<>();
            for (int i = 0; i < tableModel.getRowCount(); i++) {
                labelToField.put((String) tableModel.getValueAt(i, 0), (String) tableModel.getValueAt(i, 1));
            }
            info.setLabelToField(labelToField);
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
