package dialogs;

import com.formdev.flatlaf.FlatClientProperties;
import com.formdev.flatlaf.extras.FlatSVGIcon;
import data.Standards;
import data.Task;
import packets.CreateTask;
import packets.RequestID;
import taskglacier.MainFrame;

import javax.swing.*;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.util.ArrayList;
import java.util.List;

import static taskglacier.MainFrame.mainFrame;

public class AddTask extends JDialog {
    public static AddTask openInstance = null;

    public static List<Integer> activeRequests = new ArrayList<>();

    public AddTask(MainFrame mainFrame, Window parentWindow, int defaultParentID) {
        super(parentWindow);
        openInstance = this;

        setModalityType(ModalityType.APPLICATION_MODAL);

        // name, time tracking and project info
        // some of this info can be automatically filled
        JTextField name = new JTextField(50);
        JTextPane bulkAdd = new JTextPane();

        bulkAdd.setVisible(false);

        JTextField parent = new JTextField();
        parent.setText(String.valueOf(defaultParentID));

        JButton bulkSwitch = new JButton("^");
        JPanel flow = createFlow("Name: ", name);
        bulkSwitch.addActionListener(e -> {
            if (bulkSwitch.getText().equals("^")) {
                bulkSwitch.setText("v");
                flow.setVisible(false);
                bulkAdd.setVisible(true);
            }
        });

        JButton add = new JButton("Add");

        setTitle("Add Task");

        add.addActionListener(e -> {
            int parentID = Integer.parseInt(parent.getText());
            if (bulkSwitch.getText().equals("v")) {
                String[] names = bulkAdd.getText().split(System.lineSeparator());

                List<CreateTask> packets = new ArrayList<>();

                for (String s : names) {
                    int requestID = RequestID.nextRequestID();
                    CreateTask create = new CreateTask(s, parentID, requestID);
                    packets.add(create);

                    activeRequests.add(requestID);
                }

                for (CreateTask packet : packets) {
                    mainFrame.getConnection().sendPacket(packet);
                }
            }
            else {
                int requestID = RequestID.nextRequestID();
                CreateTask create = new CreateTask(name.getText(), parentID, requestID);

                activeRequests.add(requestID);

                mainFrame.getConnection().sendPacket(create);
            }
        });

        KeyAdapter enterAction = new KeyAdapter() {
            @Override
            public void keyPressed(KeyEvent e) {
                if (e.getKeyCode() == KeyEvent.VK_ENTER) {
                    add.doClick();
                }
            }
        };
        name.addKeyListener(enterAction);

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        setLayout(new GridBagLayout());


        add(flow, gbc);
        add(bulkAdd, gbc);
        gbc.gridx++;
        add(bulkSwitch, gbc);
        gbc.gridy++;
        gbc.gridx = 0;
        gbc.gridwidth = 2;

        JToolBar toolBar = new JToolBar();
        FlatSVGIcon searchIcon = new FlatSVGIcon(getClass().getResource("/search-svgrepo-com.svg")).derive(24, 24);

        JButton search = new JButton(searchIcon);

        toolBar.add(search);

        parent.putClientProperty(FlatClientProperties.TEXT_FIELD_TRAILING_COMPONENT, toolBar);

        add(createFlow("Parent: ", parent), gbc);

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
                parent.setText(String.valueOf(picker.task.id));
                parent.setToolTipText(picker.task.name);
            }
        });

        gbc.gridy++;

        add(add, gbc);
        gbc.gridy++;

        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }

    public void failureResponse(String message) {
        JOptionPane.showMessageDialog(this, message, "Failure", JOptionPane.ERROR_MESSAGE);
    }

    public void close() {
        openInstance = null;
        AddTask.this.dispose();
    }

    JPanel createFlow(String name, JComponent comp) {
        JPanel panel = new JPanel(new FlowLayout());
        panel.add(new JLabel(name));
        panel.add(comp);
        return panel;
    }
}
