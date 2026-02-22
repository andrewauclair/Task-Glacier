package config;

import data.Task;
import packets.RequestID;
import packets.UpdateTask;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;

public class TaskConfig extends JDialog {
    public final Sessions sessions;

    public TaskConfig(MainFrame mainFrame, Window parent, Task task) {
        super(parent);

        setModalityType(ModalityType.APPLICATION_MODAL);

        KeyStroke ESCAPE_KEY = KeyStroke.getKeyStroke(KeyEvent.VK_ESCAPE, 0);

        InputMap inputMap = ((JComponent) getContentPane()).getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW);
        ActionMap actionMap = ((JComponent) getContentPane()).getActionMap();

        inputMap.put(ESCAPE_KEY, "escape");
        actionMap.put("escape", new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                TaskConfig.this.dispose();
            }
        });

        setLayout(new GridBagLayout());

        setTitle("Task Configuration - " + task.name);

        DefaultTableModel model = new DefaultTableModel(0, 1);
        model.addRow(new Object[]{"General"});
        model.addRow(new Object[]{"Labels"});
        model.addRow(new Object[]{"Sessions"});
        model.addRow(new Object[]{"Time Entry"});

        JTable list = new JTable(model);
        list.setTableHeader(null);

        JPanel foo = new JPanel(new BorderLayout());
        JSplitPane split = new JSplitPane();
        split.setLeftComponent(new JScrollPane(list));

        CardLayout layout = new CardLayout();
        JPanel stack = new JPanel(layout);

        stack.add(new JPanel(), "");
        General general = new General(task);
        stack.add(general, "General");
        stack.add(new Labels(task), "Labels");
        sessions = new Sessions(parent, task);
        stack.add(sessions, "Sessions");
        TimeEntry timeEntry = new TimeEntry(this, mainFrame, task);
        stack.add(timeEntry, "Time Entry");
        split.setRightComponent(stack);

        layout.show(stack, "General");

        SwingUtilities.invokeLater(() -> {
            split.setDividerLocation(0.3);
            list.setRowSelectionInterval(0, 0);
        });

        list.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        list.getSelectionModel().addListSelectionListener(e -> {
            if (e.getValueIsAdjusting()) {
                return;
            }
            String name = e.getFirstIndex() != -1 ? (String) model.getValueAt(list.getSelectedRow(), 0) : "";

            layout.show(stack, name);
        });

        foo.add(split);

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridy = 0;
        gbc.gridx = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.insets = new Insets(5, 5, 5, 5);

        add(foo, gbc);

        gbc.gridy++;
        gbc.weighty = 0;
        gbc.weighty = 0;
        gbc.anchor = GridBagConstraints.SOUTHEAST;
        gbc.fill = GridBagConstraints.NONE;

        JButton save = new JButton("Save");

        add(save, gbc);

        save.addActionListener(e -> {
            if (!general.verify()) {
                return;
            }

            // send any packets that are necessary
            UpdateTask update = new UpdateTask(RequestID.nextRequestID(), task.id, Integer.parseInt(general.parent.getText()), general.description.getText());
            update.indexInParent = task.indexInParent;
            update.locked = task.locked;
            update.timeEntry = task.timeEntry;
            update.state = task.state;

            general.save(task, update);
            timeEntry.save(task, update);

            mainFrame.getConnection().sendPacket(update);

            sessions.save(mainFrame.getConnection());

            dispose();
        });

        setSize(550, 400);

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
