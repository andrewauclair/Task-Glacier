package dialogs;

import data.Task;
import me.xdrop.fuzzywuzzy.FuzzySearch;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableRowSorter;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

public class TaskSearch extends JDialog {
    class SearchRowFilter extends RowFilter {
        String regex;
        RowFilter regexFilter;
        public SearchRowFilter(String regex) {
            this.regex = regex;
            regexFilter = RowFilter.regexFilter(regex, 0);
        }

        @Override
        public boolean include(Entry entry) {
            return !regex.isEmpty() && regexFilter.include(entry);
        }
    }
    class SearchRowSorter extends TableRowSorter {

    }
    class SearchComparator implements Comparator<String> {
        String search;
        public SearchComparator(String search) {
            this.search = search;
        }
        @Override
        public int compare(String o1, String o2) {
            int ratio = FuzzySearch.ratio(search, o1);
            int ratio1 = FuzzySearch.ratio(search, o2);
            System.out.println("ratios: " + ratio + " " + ratio1);
            return Integer.compare(ratio, ratio1);
        }
    }
    public TaskSearch() {
        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.weightx = 1;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        JTextField search = new JTextField();

        DefaultTableModel model = new DefaultTableModel();
        model.addColumn("Task");

        JTable table = new JTable(model);
//        table.setTableHeader(null);

        TableRowSorter<DefaultTableModel> sorter = new TableRowSorter<>(model);
        sorter.setComparator(0, new SearchComparator(""));
        List<RowSorter.SortKey> sortKeys = new ArrayList<>();
        sortKeys.add(new RowSorter.SortKey(0, SortOrder.DESCENDING));
        sorter.setSortKeys(sortKeys);
//        RowFilter<DefaultTableModel, Object> filter = new SearchRowFilter("");
//
//        sorter.setRowFilter(filter);
        table.setRowSorter(sorter);

//        table.setAutoCreateRowSorter(true);

        add(search, gbc);

        gbc.gridy++;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(new JScrollPane(table), gbc);

        search.addKeyListener(new KeyAdapter() {
            @Override
            public void keyReleased(KeyEvent e) {
//                RowFilter<DefaultTableModel, Object> filter = new SearchRowFilter(search.getText());
//                sorter.setRowFilter(filter);
                sorter.setComparator(0, new SearchComparator(search.getText()));
                sorter.sort();
            }
        });

        for (Task task : MainFrame.mainFrame.getTaskModel().getTasks()) {
            StringBuilder text = new StringBuilder();

            List<String> parents = new ArrayList<>();

            int parentID = task.parentID;

            while (parentID != 0) {
                Task parentTask = MainFrame.mainFrame.getTaskModel().getTask(parentID);

                if (task == null) {
                    break;
                }
                parents.add(0, parentTask.name);

                parentID = task.parentID;
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
