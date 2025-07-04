package dialogs;

import data.Task;
import data.TimeData;
import org.jdesktop.swingx.painter.AbstractLayoutPainter;
import packets.TaskInfo;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.Vector;

public class TaskConfig extends JDialog {
    class LabeledComponent extends JPanel {
        LabeledComponent(JLabel label, JComponent component) {
            super(new FlowLayout());

            add(label);
            add(component);
        }
    }

    // general (id, name, status, parent, bugzilla)
    class General extends JPanel {
        General(Task task) {
            setLayout(new GridBagLayout());

            GridBagConstraints gbc = new GridBagConstraints();
            gbc.gridx = 0;
            gbc.gridy = 0;

            add(new JLabel("ID " + task.id), gbc);

            gbc.gridy++;

            JTextField name = new JTextField(20);
            add(new LabeledComponent(new JLabel("name"), name), gbc);

            name.setText(task.name);

            JComboBox<String> status = new JComboBox<>();
            status.addItem("Inactive");
            status.addItem("Active");
            status.addItem("Finished");

            gbc.gridy++;
            add(status, gbc);

            JTextField parent = new JTextField(3);
            parent.setText(String.valueOf(task.parentID));

            gbc.gridy++;
            add(parent, gbc);
        }
    }

    class Labels extends JPanel {
        Labels(Task task) {
            add(new JLabel(getClass().toGenericString()));
        }
    }

    class Sessions extends JPanel {
        Sessions(Task task) {
            add(new JLabel(getClass().toGenericString()));

            for (TaskInfo.Session session : task.sessions) {

            }
        }
    }

    class TimeEntry extends JPanel {
        private final TimeData timeData;

        JButton add = new JButton("+");
        JButton remove = new JButton("-");

        TableModel model = new TableModel();
        JTable table = new JTable(model);

        class Row {
            TimeData.TimeCategory category;
            TimeData.TimeCode code;
        }

        class TableModel extends AbstractTableModel {
            List<Row> data = new ArrayList<>();

            @Override
            public int getRowCount() {
                return data.size();
            }

            @Override
            public String getColumnName(int column) {
                if (column == 0) {
                    return "Category";
                }
                return "Code";
            }

            @Override
            public Class<?> getColumnClass(int columnIndex) {
                return String.class;
            }

            @Override
            public int getColumnCount() {
                return 2;
            }

            @Override
            public Object getValueAt(int rowIndex, int columnIndex) {
                Row row = data.get(rowIndex);
                if (columnIndex == 0) {
                    return row.category.name;
                }
                return row.code.name;
            }
        }

        class TimeEntryDialog extends JDialog {
            TimeEntryDialog() {
                super(TaskConfig.this, true);

                setLayout(new GridBagLayout());

                JButton save = new JButton("Add");
                save.setEnabled(false);

                JComboBox<String> timeCategory = new JComboBox<>();
                JComboBox<String> timeCode = new JComboBox<>();

                timeCategory.addActionListener(e -> {
                    Optional<TimeData.TimeCategory> timeCategory2 = timeData.getTimeCategories().stream()
                            .filter(timeCategory1 -> timeCategory1.name.equals(timeCategory.getSelectedItem()))
                            .findFirst();

                    timeCode.removeAllItems();
                    save.setEnabled(false);

                    if (timeCategory2.isPresent()) {
                        for (TimeData.TimeCode code : timeCategory2.get().timeCodes) {
                            timeCode.addItem(code.name);
                        }
                        timeCode.setSelectedItem(null);
                    }
                });

                timeCode.addActionListener(e -> {
                    save.setEnabled(true);
                });
                for (TimeData.TimeCategory category : timeData.getTimeCategories()) {
                    timeCategory.addItem(category.name);
                }

                timeCategory.setSelectedItem(null);

                GridBagConstraints gbc = new GridBagConstraints();
                gbc.gridx = 0;
                gbc.gridy = 0;

                add(new LabeledComponent(new JLabel("Time Category"), timeCategory), gbc);
                gbc.gridy++;
                add(new LabeledComponent(new JLabel("Time Code"), timeCode), gbc);
                gbc.gridy++;
                add(save, gbc);

                save.addActionListener(e -> {
                    Optional<TimeData.TimeCategory> timeCategory2 = timeData.getTimeCategories().stream()
                            .filter(timeCategory1 -> timeCategory1.name.equals(timeCategory.getSelectedItem()))
                            .findFirst();

                    Row row = new Row();
                    row.category = timeCategory2.get();
                    row.code = timeCategory2.get().timeCodes.stream()
                            .filter(timeCode1 -> timeCode1.name.equals(timeCode.getSelectedItem()))
                            .findFirst().get();

                    model.data.add(row);
                    model.fireTableRowsInserted(model.data.size() - 1, model.data.size() - 1);

                    dispose();
                });

                pack();

                setLocationRelativeTo(TaskConfig.this);
            }
        }

        TimeEntry(MainFrame mainFrame) {
            setLayout(new GridBagLayout());

            this.timeData = mainFrame.getTimeData();

            JPanel buttons = new JPanel(new FlowLayout());
            buttons.add(add);
            buttons.add(remove);

            GridBagConstraints gbc = new GridBagConstraints();
            gbc.gridx = 0;
            gbc.gridy = 0;

            add(buttons, gbc);

            gbc.gridy++;
            gbc.weightx = 1;
            gbc.weighty = 1;
            gbc.fill = GridBagConstraints.BOTH;
            add(new JScrollPane(table), gbc);

            add.addActionListener(e -> {
                TimeEntryDialog dialog = new TimeEntryDialog();
                dialog.setVisible(true);
                dialog.setLocationRelativeTo(mainFrame);
            });
        }
    }

    public TaskConfig(MainFrame mainFrame, Task task) {
        super(mainFrame, true);

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
        stack.add(new General(task), "General");
        stack.add(new Labels(task), "Labels");
        stack.add(new Sessions(task), "Sessions");
        stack.add(new TimeEntry(mainFrame), "Time Entry");
        split.setRightComponent(stack);

        SwingUtilities.invokeLater(() -> {
            split.setDividerLocation(0.3);
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

        add(foo, gbc);

        gbc.gridy++;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        JButton save = new JButton("Save");
        add(save, gbc);

        save.addActionListener(e -> {
            // send any packets that are necessary

            dispose();
        });

        setSize(400, 300);

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
