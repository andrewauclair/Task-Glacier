package dialogs;

import packets.PacketType;
import packets.TaskStateChange;
import taskglacier.MainFrame;
import util.LabeledComponent;

import javax.swing.*;
import java.awt.*;

public class UnspecifiedTask extends JDialog {
    public UnspecifiedTask(MainFrame mainFrame) {
        setModal(true);

        JButton create = new JButton("Create New Task");

        JTextField taskID = new JTextField(5);

        add(create);

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 0;

        add(new LabeledComponent("Task ID", taskID), gbc);

        gbc.gridy++;

        add(create, gbc);
        gbc.gridy++;

        JButton done = new JButton("Done");

        done.addActionListener(e -> {
            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.STOP_UNSPECIFIED_TASK;
            change.taskID = Integer.parseInt(taskID.getText());

            mainFrame.getConnection().sendPacket(change);

            UnspecifiedTask.this.dispose();
        });
        add(done, gbc);

        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
