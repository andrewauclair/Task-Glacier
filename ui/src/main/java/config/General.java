package config;

import com.formdev.flatlaf.FlatClientProperties;
import com.formdev.flatlaf.extras.FlatSVGIcon;
import data.Task;
import data.TaskState;
import dialogs.TaskIDFilter;
import dialogs.TaskPicker;
import packets.UpdateTask;
import util.Icons;
import util.LabeledComponent;

import javax.swing.*;
import javax.swing.text.AbstractDocument;
import java.awt.*;
import java.awt.event.FocusAdapter;
import java.awt.event.FocusEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;

import static taskglacier.MainFrame.mainFrame;

// general (id, name, status, parent, bugzilla)
class General extends JPanel {
    private final Task task;
    JTextArea description = new JTextArea(6, 20);
    JComboBox<String> status = new JComboBox<>();
    JTextField parent = new JTextField(6);
    JCheckBox serverControlled = new JCheckBox("Server Controlled");
    JCheckBox locked = new JCheckBox("Locked");

    General(Task task) {
        this.task = task;
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
        gbc.gridy++;

        description.setWrapStyleWord(true);
        description.setLineWrap(true);
        description.setText(task.name);

        status.addItem("Pending");
        status.addItem("Active");
        status.addItem("Finished");

        add(new LabeledComponent("Status", status), gbc);
        gbc.gridy++;

        switch (task.state) {
            case PENDING -> status.setSelectedItem("Pending");
            case ACTIVE -> status.setSelectedItem("Active");
            case FINISHED -> status.setSelectedItem("Finished");
        }

        JToolBar toolBar = new JToolBar();
        JButton search = new JButton(Icons.searchIcon);

        toolBar.add(search);

        parent.putClientProperty(FlatClientProperties.TEXT_FIELD_TRAILING_COMPONENT, toolBar);

        parent.setText(String.valueOf(task.parentID));
        Task parentTask = mainFrame.getTaskModel().getTask(task.parentID);
        parent.setToolTipText(parentTask != null ? parentTask.name : "Root");

        parent.addFocusListener(new FocusAdapter() {
            @Override
            public void focusLost(FocusEvent e) {
                int newTaskID = Integer.valueOf(parent.getText());
                Task parentTask = mainFrame.getTaskModel().getTask(newTaskID);

                if (newTaskID == 0) {
                    parent.setToolTipText("Root");
                }
                else if (parentTask == null) {
                    parent.setToolTipText("");
                }
                else {
                    parent.setToolTipText(parentTask.name);
                }
            }
        });
        ((AbstractDocument) parent.getDocument()).setDocumentFilter(new TaskIDFilter());

        add(new LabeledComponent("Parent", parent), gbc);
        gbc.gridy++;

        search.addActionListener(e -> {
            int taskID = 0;
            try {
                taskID = Integer.parseInt(parent.getText());
            }
            catch (NumberFormatException ignore) {
            }

            Task selectedParent = mainFrame.getTaskModel().getTask(taskID);
            TaskPicker picker = new TaskPicker(mainFrame, selectedParent);
            picker.setVisible(true);

            if (picker.task != null) {
                if (picker.task.id == task.id) {
                    JOptionPane.showMessageDialog(this, "Task cannot be its own parent", "Invalid Parent", JOptionPane.ERROR_MESSAGE);
                }
                else {
                    parent.setText(String.valueOf(picker.task.id));
                    parent.setToolTipText(picker.task.name);
                }
            }
        });

        serverControlled.setEnabled(false);
        serverControlled.setSelected(task.serverControlled);

        add(serverControlled, gbc);
        gbc.gridy++;

        locked.setSelected(task.locked);

        add(locked, gbc);
        gbc.gridy++;

        // add filler
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;
        add(new JLabel(), gbc);
        gbc.gridy++;

        locked.addActionListener(e -> {
            boolean controlsLocked = serverControlled.isSelected() || locked.isSelected();

            description.setEnabled(!controlsLocked);
            status.setEnabled(!controlsLocked);
            parent.setEnabled(!controlsLocked);
            search.setEnabled(!controlsLocked);
        });

        boolean controlsLocked = serverControlled.isSelected() || locked.isSelected();

        description.setEnabled(!controlsLocked);
        status.setEnabled(!controlsLocked);
        parent.setEnabled(!controlsLocked);
        search.setEnabled(!controlsLocked);
        locked.setEnabled(!serverControlled.isSelected());
    }

    private boolean hasChanges(Task task) {
        return !task.name.equals(description.getText()) ||
                task.parentID != Integer.parseInt(parent.getText()) ||
                task.locked != locked.isSelected();
    }

    public void save(Task task, UpdateTask update) {
        if (hasChanges(task)) {
            update.state = task.state;

            if (status.getSelectedItem().equals("Pending") && task.state == TaskState.FINISHED)
            {
                update.state = TaskState.PENDING;
            }
            update.locked = locked.isSelected();
        }
    }

    public boolean verify() {
        int parentID = Integer.parseInt(parent.getText());

        if (parentID == task.id) {
            JOptionPane.showMessageDialog(this, "Task cannot be its own parent", "Invalid Parent", JOptionPane.ERROR_MESSAGE);

            return false;
        }

        Task parentTask = mainFrame.getTaskModel().getTask(parentID);

        if (parentID != 0 && parentTask == null) {
            JOptionPane.showMessageDialog(this, "Parent task ID does not exist", "Invalid Parent", JOptionPane.ERROR_MESSAGE);

            return false;
        }

        TaskState selectedState = TaskState.PENDING;

        if (status.getSelectedItem().toString().equals("Active"))
        {
            selectedState = TaskState.ACTIVE;
        }
        else if (status.getSelectedItem().toString().equals("Finished"))
        {
            selectedState = TaskState.FINISHED;
        }

        if (selectedState != task.state && selectedState != TaskState.PENDING && task.state != TaskState.FINISHED) {
            JOptionPane.showMessageDialog(this,  "State cannot be changed unless task is finished and changing to pending", "Invalid State Selected", JOptionPane.ERROR_MESSAGE);
            return false;
        }
        return true;
    }
}
