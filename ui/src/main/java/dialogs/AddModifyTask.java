package dialogs;

import data.Standards;
import packets.CreateTask;
import packets.RequestID;
import taskglacier.MainFrame;

import javax.swing.*;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;

public class AddModifyTask extends JDialog {
    public static AddModifyTask openInstance = null;

    public void failureResponse(String message) {
        JOptionPane.showMessageDialog(this, message, "Failure", JOptionPane.ERROR_MESSAGE);
    }

    public void close() {
        openInstance = null;
        AddModifyTask.this.dispose();
    }

    JPanel createFlow(String name, JComponent comp) {
        JPanel panel = new JPanel(new FlowLayout());
        panel.add(new JLabel(name));
        panel.add(comp);
        return panel;
    }

    public AddModifyTask(MainFrame mainFrame, Window parentWindow, int parentID, boolean modify) {
        super(parentWindow);
        openInstance = this;

        setModal(true);

        // name, time tracking and project info
        // some of this info can be automatically filled
        JTextField name = new JTextField(50);

        JTextField parent = new JTextField();
        parent.setText(String.valueOf(parentID));

        JButton add = new JButton("Add");

        if (modify) {
            setTitle("Modify Task ");
        }
        else {
            setTitle("Add Task");

            add.addActionListener(e -> {
                CreateTask create = new CreateTask(name.getText(), Integer.parseInt(parent.getText()), RequestID.nextRequestID());
                mainFrame.getConnection().sendPacket(create);
            });

            // TODO we'll have to add this to all components on the dialog
            name.addKeyListener(new KeyAdapter() {
                @Override
                public void keyPressed(KeyEvent e) {
                    if (e.getKeyCode() == KeyEvent.VK_ENTER) {
                        add.doClick();
                    }
                }
            });
        }

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        setLayout(new GridBagLayout());

        add(createFlow("Name: ", name), gbc);
        gbc.gridy++;

        add(createFlow("Parent: ", parent), gbc);
        gbc.gridy++;

        add(add, gbc);
        gbc.gridy++;

        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
