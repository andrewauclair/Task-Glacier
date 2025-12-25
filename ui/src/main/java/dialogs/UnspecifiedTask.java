package dialogs;

import com.formdev.flatlaf.FlatClientProperties;
import com.formdev.flatlaf.extras.FlatSVGIcon;
import packets.PacketType;
import packets.TaskStateChange;
import taskglacier.MainFrame;
import util.LabeledComponent;

import javax.swing.*;
import javax.swing.text.AbstractDocument;
import java.awt.*;

public class UnspecifiedTask extends JDialog {
    public UnspecifiedTask(MainFrame mainFrame) {
        setModal(true);

        setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);

        JButton create = new JButton("Create New Task");

        create.addActionListener(e -> {
            AddTask add = new AddTask(mainFrame, this, 0);
            add.setVisible(true);
        });

        JTextField taskID = new JTextField(6);

        JToolBar toolBar = new JToolBar();
        FlatSVGIcon searchIcon = new FlatSVGIcon(getClass().getResource("/search-svgrepo-com.svg")).derive(24, 24);

        JButton search = new JButton(searchIcon);

        toolBar.add(search);

        taskID.putClientProperty(FlatClientProperties.TEXT_FIELD_TRAILING_COMPONENT, toolBar);

        add(create);

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 0;

        ((AbstractDocument) taskID.getDocument()).setDocumentFilter(new TaskIDFilter());

        add(new LabeledComponent("Task ID", taskID), gbc);
        gbc.gridy++;

        search.addActionListener(e -> {
            TaskPicker picker = new TaskPicker(mainFrame);
            picker.setVisible(true);

            if (picker.task != null) {
                taskID.setText(String.valueOf(picker.task.id));
                taskID.setToolTipText(picker.task.name);
            }
        });
        gbc.gridy++;

        add(create, gbc);
        gbc.gridy++;

        JButton done = new JButton("Done");

        done.addActionListener(e -> {
            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.STOP_UNSPECIFIED_TASK;
            change.taskID = Integer.parseInt(taskID.getText());

            if (mainFrame.isConnected()) {
                mainFrame.getConnection().sendPacket(change);
            }
            
            mainFrame.getTaskModel().removeUnspecifiedTask();

            mainFrame.getSystemTrayDisplay().setUnspecifiedTaskState(true);

            UnspecifiedTask.this.dispose();
        });
        add(done, gbc);

        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
