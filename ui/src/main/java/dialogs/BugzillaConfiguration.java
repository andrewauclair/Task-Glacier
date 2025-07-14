package dialogs;

import data.Standards;
import packets.BugzillaInfo;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.util.HashMap;
import java.util.Map;

public class BugzillaConfiguration extends JDialog {
    private DefaultTableModel instanceModel = new DefaultTableModel(0, 1);
    private JTable instanceTable = new JTable(instanceModel);

    private JButton instanceAdd = new JButton("+");
    private JButton instanceRemove = new JButton("-");

    private JButton save = new JButton("Save");

    class BugzillaInstance {
        int instanceID;
        String name;
        JTextField URL = new JTextField();
        JTextField apiKey = new JTextField();
        JTextField username = new JTextField();
        JTextField rootTask = new JTextField();
        DefaultTableModel groupByModel = new DefaultTableModel(new Object[] { "Group By" }, 0);
        DefaultTableModel labelModel = new DefaultTableModel(new Object[] { "Label", "Bugzilla Field"}, 0);
    }
    private Map<String, BugzillaInstance> instances = new HashMap<>();

    private JPanel buildInstances() {

        instanceTable.setTableHeader(null);

        GridBagConstraints gbc = new GridBagConstraints();

        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        gbc.gridx = 0;
        gbc.gridy = 0;

        gbc.gridwidth = 1;

        JPanel panel = new JPanel(new GridBagLayout());

        panel.add(instanceAdd, gbc);
        gbc.gridx++;
        panel.add(instanceRemove, gbc);

        gbc.gridx = 0;
        gbc.gridy++;
        gbc.gridwidth = 2;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        instanceTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        panel.add(instanceTable, gbc);

        return panel;
    }

    private JPanel createInstance(String name, BugzillaInstance instance) {
        instance.name = name;

        JPanel panel = new JPanel(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();

        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.weightx = 1;

        JPanel general = new JPanel(new GridBagLayout());
        JPanel groupBy = new JPanel(new GridBagLayout());
        JPanel labels = new JPanel(new GridBagLayout());


        general.add(new LabeledComponent("URL", instance.URL), gbc);
        gbc.gridy++;


        general.add(new LabeledComponent("API Key", instance.apiKey), gbc);
        gbc.gridy++;

        // TODO currently we'll do all searching by 'assigned_to' in the bugzilla search. In the future we should support searching by other fields as well, such as component.

        general.add(new LabeledComponent("Username", instance.username), gbc);
        gbc.gridy++;

        gbc.fill = GridBagConstraints.NONE;

        general.add(new LabeledComponent("Root Task", instance.rootTask), gbc);
        gbc.gridy++;

        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        general.add(new JLabel(), gbc);

        // configure search parameters and task arrangement
        // configure labels
        JTable groupByTable = new JTable(instance.groupByModel);

        instance.groupByModel.addRow(new Object[] { "target_milestone" });

        JTable labelTable = new JTable(instance.labelModel);
        labelTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        instance.labelModel.addRow(new Object[] { "Priority", "priority" });
        instance.labelModel.addRow(new Object[] { "Status", "status" });
        instance.labelModel.addRow(new Object[] { "Resolution", "resolution" });
        instance.labelModel.addRow(new Object[] { "Severity", "severity" });
        instance.labelModel.addRow(new Object[] { "Target", "target_milestone" });
        instance.labelModel.addRow(new Object[] { "Component", "component" });

        JButton groupByAdd = new JButton("+");
        JButton groupByRemove = new JButton("-");

        JButton groupByMoveUp = new JButton("^");
        JButton groupByMoveDown = new JButton("v");

        groupByRemove.setEnabled(false);
        groupByMoveUp.setEnabled(false);
        groupByMoveDown.setEnabled(false);

        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;

        groupByAdd.addActionListener(e -> {
            String newGroupBy = JOptionPane.showInputDialog(this, "New Group By");

            instance.groupByModel.addRow(new Object[] { newGroupBy });
            instance.groupByModel.fireTableRowsInserted(instance.groupByModel.getRowCount() - 1, instance.groupByModel.getRowCount() - 1);
        });

        groupByTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        groupByTable.getSelectionModel().addListSelectionListener(e -> {
            groupByRemove.setEnabled(groupByTable.getSelectedRow() != -1);
            groupByMoveUp.setEnabled(groupByTable.getSelectedRow() != -1 && groupByTable.getSelectedRow() != 0);
            groupByMoveDown.setEnabled(groupByTable.getSelectedRow() != -1 && groupByTable.getSelectedRow() != instance.groupByModel.getRowCount() - 1);
        });

        groupByRemove.addActionListener(e -> {
            instance.groupByModel.removeRow(groupByTable.getSelectedRow());
            instance.groupByModel.fireTableRowsInserted(groupByTable.getSelectedRow(), groupByTable.getSelectedRow());
        });

        groupByMoveUp.addActionListener(e -> {
            Object value1 = instance.groupByModel.getValueAt(groupByTable.getSelectedRow() - 1, 0);
            Object value2 = instance.groupByModel.getValueAt(groupByTable.getSelectedRow(), 0);

            instance.groupByModel.setValueAt(value2, groupByTable.getSelectedRow() - 1, 0);
            instance.groupByModel.setValueAt(value1, groupByTable.getSelectedRow(), 0);

            instance.groupByModel.fireTableRowsUpdated(groupByTable.getSelectedRow() - 1, groupByTable.getSelectedRow());
            groupByTable.setRowSelectionInterval(groupByTable.getSelectedRow() - 1, groupByTable.getSelectedRow() - 1);
        });

        groupByMoveDown.addActionListener(e -> {
            Object value1 = instance.groupByModel.getValueAt(groupByTable.getSelectedRow(), 0);
            Object value2 = instance.groupByModel.getValueAt(groupByTable.getSelectedRow() + 1, 0);

            instance.groupByModel.setValueAt(value2, groupByTable.getSelectedRow(), 0);
            instance.groupByModel.setValueAt(value1, groupByTable.getSelectedRow() + 1, 0);

            instance.groupByModel.fireTableRowsUpdated(groupByTable.getSelectedRow(), groupByTable.getSelectedRow() + 1);
            groupByTable.setRowSelectionInterval(groupByTable.getSelectedRow() + 1, groupByTable.getSelectedRow() + 1);
        });

        groupBy.add(groupByAdd, gbc);
        gbc.gridx++;
        groupBy.add(groupByRemove, gbc);

        gbc.gridx = 0;
        gbc.gridy++;
        gbc.gridwidth = 2;
        gbc.gridheight = 2;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;
        groupBy.add(new JScrollPane(groupByTable), gbc);

        gbc.gridx++;
        gbc.gridx++;
        gbc.gridwidth = 1;
        gbc.gridheight = 1;
        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;

        groupBy.add(groupByMoveUp, gbc);
        gbc.gridy++;
        groupBy.add(groupByMoveDown, gbc);

        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.gridwidth = 1;
        gbc.gridheight = 1;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;
        labels.add(new JScrollPane(labelTable), gbc);

        JTabbedPane tabs = new JTabbedPane();
        tabs.add("General", general);
        tabs.add("Group By", groupBy);
        tabs.add("Labels", labels);

        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        panel.add(tabs, gbc);

        return panel;
    }

    private JPanel createBlankPanel() {
        JPanel panel = new JPanel(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        panel.add(new JLabel(), gbc);

        return panel;
    }

    public BugzillaConfiguration(MainFrame mainFrame) {
        setLayout(new GridBagLayout());
        setTitle("Bugzilla Configuration");
        setModal(true);

        JSplitPane split = new JSplitPane();
        split.setLeftComponent(buildInstances());

        CardLayout layout = new CardLayout();
        JPanel stack = new JPanel(layout);

        stack.add(createBlankPanel(), "blank");

        split.setRightComponent(stack);

        instanceAdd.addActionListener(e -> {
            String name = JOptionPane.showInputDialog(this, "New Bugzilla Instance Name");

            BugzillaInstance instance = new BugzillaInstance();
            instances.put(name, instance);

            JPanel instancePanel = createInstance(name, instance);

            stack.add(instancePanel, name);

            instanceModel.addRow(new String[] { name });
            instanceModel.fireTableRowsInserted(instanceModel.getRowCount() - 1, instanceModel.getRowCount() - 1);

            instanceTable.getSelectionModel().setSelectionInterval(instanceModel.getRowCount() - 1, instanceModel.getRowCount() - 1);
        });

        instanceTable.getSelectionModel().addListSelectionListener(e -> {
            instanceRemove.setEnabled(instanceTable.getSelectedRow() != -1);

            String name = instanceTable.getSelectedRow() != -1 ? (String) instanceModel.getValueAt(instanceTable.getSelectedRow(), 0) : "blank";

            layout.show(stack, name);
        });

        instanceRemove.setEnabled(false);

        instanceRemove.addActionListener(e -> {
            if (instanceTable.getSelectedRow() != -1) {
                String name = (String) instanceModel.getValueAt(instanceTable.getSelectedRow(), 0);

                instanceModel.removeRow(instanceTable.getSelectedRow());
                instanceModel.fireTableRowsDeleted(instanceTable.getSelectedRow(), instanceTable.getSelectedRow());

                instances.remove(name);

                layout.show(stack, "blank");
            }
        });

        GridBagConstraints gbc = new GridBagConstraints();

        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(split, gbc);

        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;
        gbc.anchor = GridBagConstraints.SOUTHEAST;
        gbc.gridy++;

        add(save, gbc);

        save.addActionListener(e -> {
            for (BugzillaInstance instance : instances.values()) {
                BugzillaInfo info = new BugzillaInfo(instance.instanceID, instance.name, instance.URL.getText(), instance.apiKey.getText(), instance.username.getText());

                for (int i = 0; i < instance.groupByModel.getRowCount(); i++) {
                    info.groupTasksBy.add((String) instance.groupByModel.getValueAt(i, 0));
                }

                info.setRootTaskID(Integer.parseInt(instance.rootTask.getText()));
                Map<String, String> labelToField = new HashMap<>();
                for (int i = 0; i < instance.labelModel.getRowCount(); i++) {
                    labelToField.put((String) instance.labelModel.getValueAt(i, 0), (String) instance.labelModel.getValueAt(i, 1));
                }
                info.setLabelToField(labelToField);
                mainFrame.getConnection().sendPacket(info);
            }

            BugzillaConfiguration.this.dispose();
        });

        for (BugzillaInfo info : MainFrame.bugzillaInfo.values()) {
            BugzillaInstance instance = new BugzillaInstance();
            instance.name = info.name;
            instance.URL.setText(info.url);
            instance.apiKey.setText(info.apiKey);
            instance.username.setText(info.username);
            instance.rootTask.setText(String.valueOf(info.rootTaskID));

            instances.put(info.name, instance);

            stack.add(createInstance(info.name, instance), info.name);

            instanceModel.addRow(new Object[] { info.name });
            instanceModel.fireTableRowsInserted(instanceModel.getRowCount() - 1, instanceModel.getRowCount() - 1);

            if (instanceModel.getRowCount() == 1) {
                instanceTable.getSelectionModel().setSelectionInterval(instanceModel.getRowCount() - 1, instanceModel.getRowCount() - 1);
            }

            instance.groupByModel.setRowCount(0);
            instance.groupByModel.fireTableDataChanged();

            for (String groupBy : info.groupTasksBy) {
                instance.groupByModel.addRow(new Object[] { groupBy });
                instance.groupByModel.fireTableRowsInserted(instance.groupByModel.getRowCount() - 1, instance.groupByModel.getRowCount() - 1);
            }

            instance.labelModel.setRowCount(0);
            instance.labelModel.fireTableDataChanged();

            info.labelToField.forEach((label, field) -> {
                instance.labelModel.addRow(new Object[] { label, field });
                instance.labelModel.fireTableRowsInserted(instance.labelModel.getRowCount() - 1, instance.labelModel.getRowCount() - 1);
            });
        }



        pack();

        setSize(400, 300);

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
