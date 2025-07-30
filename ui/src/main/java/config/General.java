package config;

import data.Task;
import packets.UpdateTask;
import util.LabeledComponent;

import javax.swing.*;
import java.awt.*;

// general (id, name, status, parent, bugzilla)
class General extends JPanel {
    JTextArea description = new JTextArea(6, 20);
    JTextField parent = new JTextField(3);
    JCheckBox serverControlled = new JCheckBox("Server Controlled");
    JCheckBox locked = new JCheckBox("Locked");

    General(Task task) {
        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 0;

        add(new JLabel("ID " + task.id), gbc);

        gbc.gridy++;
        add(new LabeledComponent("Description", description, GridBagConstraints.NORTH), gbc);

        description.setWrapStyleWord(true);
        description.setLineWrap(true);
        description.setText(task.name);

        JComboBox<String> status = new JComboBox<>();
        status.addItem("Pending");
        status.addItem("Active");
        status.addItem("Finished");

        gbc.gridy++;
        add(new LabeledComponent("Status", status), gbc);

        switch (task.state) {
            case PENDING -> status.setSelectedItem("Pending");
            case ACTIVE -> status.setSelectedItem("Active");
            case FINISHED -> status.setSelectedItem("Finished");
        }

        parent.setText(String.valueOf(task.parentID));

        gbc.gridy++;
        add(new LabeledComponent("Parent", parent), gbc);


        serverControlled.setEnabled(false);
        serverControlled.setSelected(task.serverControlled);

        gbc.gridy++;
        add(serverControlled, gbc);

        locked.setSelected(task.locked);

        gbc.gridy++;
        add(locked, gbc);

        // add filler
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;
        add(new JLabel(), gbc);

        locked.addActionListener(e -> {
            boolean controlsLocked = serverControlled.isSelected() || locked.isSelected();

            description.setEnabled(!controlsLocked);
            status.setEnabled(!controlsLocked);
            parent.setEnabled(!controlsLocked);
        });

        boolean controlsLocked = serverControlled.isSelected() || locked.isSelected();

        description.setEnabled(!controlsLocked);
        status.setEnabled(!controlsLocked);
        parent.setEnabled(!controlsLocked);
        locked.setEnabled(!serverControlled.isSelected());
    }

    private boolean hasChanges(Task task) {
        return !task.name.equals(description.getText()) ||
                task.parentID != Integer.parseInt(parent.getText()) ||
                task.locked != locked.isSelected();
    }

    public void save(Task task, UpdateTask update) {
        if (hasChanges(task)) {
            update.locked = locked.isSelected();
        }
    }
}
