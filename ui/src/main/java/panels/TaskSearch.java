package panels;

import data.Task;
import data.TaskState;
import me.xdrop.fuzzywuzzy.FuzzySearch;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableRowSorter;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.util.ArrayList;
import java.util.List;

public class TaskSearch extends JPanel {
    class SearchRowFilter extends RowFilter {
        String search;
        int ratio;

        public SearchRowFilter(String search, int ratio) {
            this.search = search;
            this.ratio = ratio;
        }

        @Override
        public boolean include(Entry entry) {
            String value = (String) entry.getValue(0);

            return FuzzySearch.partialRatio(search, value) > ratio;
        }
    }

    class ConfigDialog extends JDialog {
        ConfigDialog() {
            setLayout(new GridBagLayout());

            setSize(200, 200);
            setLocationRelativeTo(TaskSearch.this);

            GridBagConstraints gbc = new GridBagConstraints();
            gbc.gridx = 0;
            gbc.gridy = 0;
            gbc.anchor = GridBagConstraints.NORTHWEST;
            gbc.weightx = 1;
            gbc.fill = GridBagConstraints.NONE;

            JCheckBox displayFinished = new JCheckBox("Display Finished Tasks");
            add(displayFinished, gbc);
            gbc.gridy++;

            JButton save = new JButton("Save");
            add(save, gbc);

            save.addActionListener(e -> {
                displayFinishedTasks = displayFinished.isSelected();
                updateTasks();
                ConfigDialog.this.dispose();
            });
        }
    }
    private MainFrame mainFrame;
    private JTextField search = new JTextField();
    private JButton config = new JButton("X");

    private boolean displayFinishedTasks = false;

    DefaultTableModel model = new DefaultTableModel() {
        @Override
        public boolean isCellEditable(int row, int column) {
            return false;
        }
    };
    JTable table = new JTable(model);

    public TaskSearch(MainFrame mainFrame) {
        this.mainFrame = mainFrame;

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.weightx = 1;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        model.addColumn("Task");

        table.setTableHeader(null);

        TableRowSorter<DefaultTableModel> sorter = new TableRowSorter<>(model);
        RowFilter<DefaultTableModel, Object> filter = new SearchRowFilter("", 50);

        sorter.setRowFilter(filter);
        table.setRowSorter(sorter);

        add(search, gbc);
        gbc.gridx++;
        add(config, gbc);
        gbc.gridx = 0;

        gbc.gridy++;
        gbc.gridwidth = 2;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(new JScrollPane(table), gbc);

        search.addKeyListener(new KeyAdapter() {
            @Override
            public void keyReleased(KeyEvent e) {
                RowFilter<DefaultTableModel, Object> filter = new SearchRowFilter(search.getText(), 50);
                sorter.setRowFilter(filter);
                sorter.allRowsChanged();
            }
        });

        config.addActionListener(e -> {
            new ConfigDialog().setVisible(true);
        });
        updateTasks();
    }

    public void updateTasks() {
        model.setRowCount(0);

        for (Task task : mainFrame.getTaskModel().getTasks()) {
            if (!displayFinishedTasks && task.state == TaskState.FINISHED) {
                continue;
            }

            StringBuilder text = new StringBuilder();

            List<String> parents = new ArrayList<>();

            int parentID = task.parentID;

            while (parentID != 0) {
                Task parentTask = mainFrame.getTaskModel().getTask(parentID);

                if (task == null) {
                    break;
                }
                parents.add(0, parentTask.name);

                parentID = parentTask.parentID;
            }
            for (String parent : parents) {
                text.append(parent);
                text.append(" / ");
            }
            text.append(task.name);

            model.addRow(new Object[] { text.toString() });

        }
        model.fireTableDataChanged();
    }
}
