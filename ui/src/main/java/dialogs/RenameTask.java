package dialogs;

import packets.RequestID;
import packets.UpdateTask;
import taskglacier.MainFrame;

import javax.swing.*;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;

public class RenameTask extends JDialog {
    public RenameTask(MainFrame mainFrame, int taskID, String name) {
        setTitle("Rename Task");

        setLayout(new FlowLayout(

        ));
        JTextField taskName = new JTextField(name);

        add(taskName);

        JButton rename = new JButton("Rename");

        add(rename);

        rename.addActionListener(e -> {
            UpdateTask update = new UpdateTask(taskID, RequestID.nextRequestID(), taskName.getText());
            mainFrame.getConnection().sendPacket(update);

            RenameTask.this.dispose();
        });

        // TODO we'll have to add this to all components on the dialog
        taskName.addKeyListener(new KeyAdapter() {
            @Override
            public void keyPressed(KeyEvent e) {
                if (e.getKeyCode() == KeyEvent.VK_ENTER) {
                    rename.doClick();
                }
            }
        });
        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
