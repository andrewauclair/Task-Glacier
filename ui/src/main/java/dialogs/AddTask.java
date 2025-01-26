package dialogs;

import data.Standards;
import packets.CreateTask;
import packets.RequestID;
import taskglacier.MainFrame;

import javax.swing.*;
import java.awt.*;

public class AddTask extends JDialog {
    public AddTask(MainFrame mainFrame) {
        // name, time tracking and project info
        // some of this info can be automatically filled (and maybe locked) in the future based on the list we're adding to
        // right now lists don't exist
        JTextField name = new JTextField(50);

        JTextField parent = new JTextField();
        parent.setText("0");

        JButton add = new JButton("Add");
        add.addActionListener(e -> {
            CreateTask create = new CreateTask(name.getText(), Integer.parseInt(parent.getText()), RequestID.nextRequestID());
            mainFrame.getConnection().sendPacket(create);
            AddTask.this.dispose();
        });

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        setLayout(new GridBagLayout());

        add(createFlow("Name:", name), gbc);
        gbc.gridy++;

        add(createFlow("Parent: ", parent), gbc);
        gbc.gridy++;

        add(add, gbc);
        gbc.gridy++;

        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }

    JPanel createFlow(String name, JComponent comp) {
        JPanel panel = new JPanel(new FlowLayout());
        panel.add(new JLabel(name));
        panel.add(comp);
        return panel;
    }
}
