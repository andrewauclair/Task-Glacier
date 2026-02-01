package dialogs;

import com.formdev.flatlaf.FlatClientProperties;
import com.formdev.flatlaf.extras.FlatSVGIcon;
import data.Task;
import packets.PacketType;
import packets.RequestID;
import packets.TaskStateChange;
import taskglacier.MainFrame;
import util.LabeledComponent;

import javax.swing.*;
import javax.swing.text.AbstractDocument;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;

import static taskglacier.MainFrame.mainFrame;

public class UnspecifiedTask extends JDialog {
    public static UnspecifiedTask openInstance = null;

    public static int requestID = 0;
    private final JTextField taskID = new JTextField(6);
    private final JButton done = new JButton("Done");

    public UnspecifiedTask(MainFrame mainFrame) {
        super(mainFrame);

        openInstance = this;

        setModalityType(ModalityType.APPLICATION_MODAL);

        setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);

        JButton create = new JButton("Create New Task...");

        create.addActionListener(e -> {
            AddTask add = new AddTask(mainFrame, this, 0, false);
            add.setVisible(true);
        });

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

        add(create, gbc);
        gbc.gridy++;

        done.setEnabled(false);

        search.addActionListener(e -> {
            int id = 0;
            try {
                id = Integer.parseInt(taskID.getText());
            }
            catch (NumberFormatException ignore) {
            }

            Task selectedParent = mainFrame.getTaskModel().getTask(id);
            TaskPicker picker = new TaskPicker(mainFrame, selectedParent);
            picker.setVisible(true);

            if (picker.task != null) {
                taskID.setText(String.valueOf(picker.task.id));
                taskID.setToolTipText(picker.task.name);

                done.setEnabled(true);
            }
        });

        taskID.addKeyListener(new KeyAdapter() {
            @Override
            public void keyReleased(KeyEvent e) {
                done.setEnabled(!taskID.getText().isEmpty());
            }
        });

        done.addActionListener(e -> {
            requestID = RequestID.nextRequestID();

            TaskStateChange change = new TaskStateChange();
            change.packetType = PacketType.STOP_UNSPECIFIED_TASK;
            change.requestID = requestID;
            change.taskID = Integer.parseInt(taskID.getText());

            if (mainFrame.isConnected()) {
                mainFrame.getConnection().sendPacket(change);
            }
            
            mainFrame.getTaskModel().removeUnspecifiedTask();

            mainFrame.getSystemTrayDisplay().setUnspecifiedTaskState(true);
        });
        add(done, gbc);

        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }

    public void close() {
        requestID = 0;
        openInstance = null;
        dispose();
    }

    public void setSelectedTask(int taskID) {
        Task task = mainFrame.getTaskModel().getTask(taskID);

        this.taskID.setText(String.valueOf(task.id));
        this.taskID.setToolTipText(task.name);

        done.setEnabled(true);
    }
}
