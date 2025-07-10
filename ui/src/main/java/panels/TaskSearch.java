package panels;

import data.Task;
import data.TaskState;
import me.xdrop.fuzzywuzzy.FuzzySearch;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableRowSorter;
import javax.swing.tree.TreePath;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import java.util.List;

public class TaskSearch extends JPanel implements TaskContextMenu.TaskDisplay {
    class SearchRowFilter extends RowFilter {
        String search;
        int ratio;

        public SearchRowFilter(String search, int ratio) {
            this.search = search;
            this.ratio = ratio;
        }

        @Override
        public boolean include(Entry entry) {
            if (!displayFinishedTasks && entry.getValue(1) == TaskState.FINISHED) {
                return false;
            }
            String value = (String) entry.getValue(0);

            return FuzzySearch.partialRatio(search, value) > ratio;
        }
    }

    class ConfigDialog extends JDialog {
        ConfigDialog() {
            super(parent);

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

    class TableModel extends AbstractTableModel {
        List<Task> tasks = new ArrayList<>();

        @Override
        public int getRowCount() {
            return tasks.size();
        }

        @Override
        public int getColumnCount() {
            return 2;
        }

        @Override
        public boolean isCellEditable(int row, int column) {
            return false;
        }

        @Override
        public Object getValueAt(int rowIndex, int columnIndex) {
            Task task = tasks.get(rowIndex);

            if (columnIndex == 0) {
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

                return text.toString();
            }
            return task.state;
        }
    }

    private MainFrame mainFrame;
    private Window parent;
    private JTextField search = new JTextField();
    private JButton config = new JButton("X");

    private boolean displayFinishedTasks = false;

    private final TableModel model = new TableModel();
    JTable table = new JTable(model);

    private final TaskContextMenu contextMenu;

    public TaskSearch(MainFrame mainFrame, Window parent, boolean displayConfigOption) {
        this.mainFrame = mainFrame;
        this.parent = parent;

        contextMenu = new TaskContextMenu(mainFrame, parent, this);

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.weightx = 1;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        table.setTableHeader(null);
        table.removeColumn(table.getColumnModel().getColumn(1));

        TableRowSorter<TableModel> sorter = new TableRowSorter<>(model);
        RowFilter<TableModel, Object> filter = new SearchRowFilter("", 50);

        sorter.setRowFilter(filter);
        table.setRowSorter(sorter);

        add(search, gbc);

        if (displayConfigOption) {
            gbc.gridx++;
            add(config, gbc);
            gbc.gridx = 0;
        }

        gbc.gridy++;
        gbc.gridwidth = 2;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(new JScrollPane(table), gbc);

        search.addKeyListener(new KeyAdapter() {
            @Override
            public void keyReleased(KeyEvent e) {
                RowFilter<TableModel, Object> filter = new SearchRowFilter(search.getText(), 50);
                sorter.setRowFilter(filter);
                sorter.allRowsChanged();
            }
        });

        config.addActionListener(e -> {
            new ConfigDialog().setVisible(true);
        });

        table.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (SwingUtilities.isRightMouseButton(e)) {
                    contextMenu.show(table, e.getX(), e.getY());
                }
                else if (e.getClickCount() == 2 && SwingUtilities.isLeftMouseButton(e)) {
                    contextMenu.openConfigDialog();
                }
            }
        });

        updateTasks();
    }

    public void updateTasks() {
        model.tasks.clear();

        for (Task task : mainFrame.getTaskModel().getTasks()) {
            model.tasks.add(task);
        }
        model.fireTableDataChanged();
    }

    @Override
    public Task taskForSelectedRow() {
        return model.tasks.get(table.convertRowIndexToModel(table.getSelectedRow()));
    }
}
